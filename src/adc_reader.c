#include "adc_reader.h"
#include "mqtt.h"

#define eos(s) ((s) + strlen(s))
#define BATTERY_ADC_GAIN 3 // We meassure a fraction of the battery voltage

static const char *TAG = "adc_reader";


const int IRRADIATION_ADC_INDEX = 0;
const int BATTERY_ADC_INDEX = 1;
const int BIAS_ADC_INDEX = 2; // used for irradiation, always keep it with the highest index
const int NFIELDS = 2; // number of entries of adc_params to read to send to the MQTT broker 

esp_err_t power_pin_down(void);
esp_err_t power_pin_up(void);

// inizialise for each adc its parameters
static struct adc_config_params adc_params[N_ADC] = {
    // solar panel params
    {
        .window_size = CONFIG_WINDOW_SIZE_IRRAD,
        .sample_frequency = CONFIG_SAMPLE_FREQ_IRRAD,
        .send_frenquency = CONFIG_SEND_FREQ_IRRAD,
        .n_samples = CONFIG_N_SAMPLES_IRRAD,
        .channel = ADC1_CHANNEL_0,
        .influxdb_field = FIELD_IRRADIATION,
        .get_mv = get_irradiation_mv,
        .last_mean = 0,

    },
    // battery params
    {
        .window_size = CONFIG_WINDOW_SIZE_BATTERY,
        .sample_frequency = CONFIG_SAMPLE_FREQ_BATTERY,
        .send_frenquency = CONFIG_SEND_FREQ_BATTERY,
        .n_samples = CONFIG_N_SAMPLES_BATTERY,
        .channel = ADC1_CHANNEL_1,
        .influxdb_field = FIELD_BATTERY,
        .get_mv = get_battery_mv,
        .last_mean = 0,

    },
    // bias params. Many of its parameters are not used, it is used to calculate irradiation value
    {
        .window_size = CONFIG_WINDOW_SIZE_IRRAD,
        .sample_frequency = CONFIG_SAMPLE_FREQ_IRRAD,
        .send_frenquency = CONFIG_SEND_FREQ_IRRAD,
        .n_samples = CONFIG_N_SAMPLES_IRRAD,
        .channel = ADC1_CHANNEL_6,
        .influxdb_field = "",
        .get_mv = get_adc_mv,
        .last_mean = 0,
    },
};

struct send_sample_buffer adcs_send_buffers[N_ADC_MEASURES] = {
    {
        .ini = 0,
        .cont = 0,
    },
    {
        .ini = 0,
        .cont = 0,
    },
};

esp_timer_handle_t sampling_timer[N_ADC_MEASURES];
esp_timer_create_args_t sample_timer_args[] = {
    {
        .callback = &sampling_timer_callback,
        .name = "sampling_timer_irra_adc",
        .arg = (void *)&IRRADIATION_ADC_INDEX,
    },
   {
        .callback = &sampling_timer_callback,
        .name = "sampling_timer_battery_adc",
        .arg = (void *)&BATTERY_ADC_INDEX,
    },
};
esp_timer_handle_t broker_sender_timer[N_ADC_MEASURES];
esp_timer_create_args_t broker_sender_timer_args[] = {
    {
        .callback = &broker_sender_callback,
        .name = "broker_timer_adc",
        .arg = (void *)&NFIELDS,
    },
//    {
//        .callback = &broker_sender_callback,
//        .name = "broker_timer_battery_adc",
//        .arg = (void *)&BATTERY_ADC_INDEX,
//    },
};


int get_adc_mv(int *value, int adc_index) {
    int adc_val = adc1_get_raw(adc_params[adc_index].channel);
    *value = esp_adc_cal_raw_to_voltage(adc_val, &adc_params[adc_index].adc_chars);
    return 0;
}

int get_battery_mv(int *value, int adc_index) {
    get_adc_mv(value, IRRADIATION_ADC_INDEX);
	*value *= BATTERY_ADC_GAIN;
	return 0;
}


int get_irradiation_mv(int *value, int adc_index) {
    int panel_mv, bias_mv;
    get_adc_mv(&panel_mv, IRRADIATION_ADC_INDEX);
    get_adc_mv(&bias_mv, BIAS_ADC_INDEX);
    *value = panel_mv - bias_mv;
    return 0;
}


static void sampling_timer_callback(void * args){
    int *adc_index = (int *) args;

    int data, sample = 0;
    if (*adc_index == IRRADIATION_ADC_INDEX)
        power_pin_up();
    for(int i= 0 ; i < adc_params[*adc_index].n_samples; i++){
        if (adc_params[*adc_index].get_mv(&data, *adc_index))
            ESP_LOGE(TAG, "Error reading ADC with index %d", *adc_index);
        else
            sample += data;
    }
    sample = (int) sample / adc_params[*adc_index].n_samples;
    if (*adc_index == IRRADIATION_ADC_INDEX)
        power_pin_down();
    ESP_LOGI(TAG, "Sample from ADC(%d) = %d", *adc_index, sample);    
    
    //Save the taken sample in the circular buffer
    adcs_send_buffers[*adc_index].samples[ adcs_send_buffers[*adc_index].cont % adc_params[*adc_index].window_size   ] = sample;
    adcs_send_buffers[*adc_index].cont++;
}

static char* buildInfluxDBString(int nfields) {
    char* str = (char*) malloc(MAX_INFLUXDB_STRING);
    sprintf(str,INFLUXDB_MEASUREMENT);
    sprintf(eos(str),INFLUXDB_LOCATION);
    for (int i=0; i < (nfields-1); i++ )
        sprintf(eos(str),"%s=%d,",adc_params[i].influxdb_field,adc_params[i].last_mean);
    
    // last field without comma
    sprintf(eos(str),"%s=%d ",adc_params[nfields-1].influxdb_field,adc_params[nfields-1].last_mean);

    // TODO include timestamp in nanoseconds

    return str;

}
//
static void broker_sender_callback(void * args){
    int *adc_index = (int *) args;
    int nsamples;
    int nfields = *adc_index;
    int i;
    char* payload;

    //See if there are samples to send
    for (i=0; i < nfields; i++) {
        if (adcs_send_buffers[i].cont > 0){
            int mean = 0;

            nsamples  = (adcs_send_buffers[i].cont > adc_params[i].window_size  )?adc_params[i].window_size :adcs_send_buffers[i].cont;
            // made the sample mean
            for (int j = 0; j < nsamples; j++)
                mean += adcs_send_buffers[i].samples[j];

            mean = mean / nsamples;
            adc_params[i].last_mean =mean;
            adcs_send_buffers[i].cont = 0;
        } 
        else {
            adc_params[i].last_mean =-1;          
        }
    }

    if ( (payload=buildInfluxDBString(nfields) ) == NULL  ) {
        ESP_LOGW(TAG, "There are still not data to send\n");
        return;
    }

    //sprintf(adcs_send_buffers[*adc_index].payload, "%d", mean);
    ESP_LOGI(TAG, "Send it to the broker: %s \n", payload );
    mqtt_send_data(MQTT_TOPIC, payload, 0, 1, 0);
    free(payload);
}


esp_err_t power_pin_setup(void) {
    gpio_config_t io_conf;
    
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = (1ULL << POWER_PIN);
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    return gpio_config(&io_conf);
}


esp_err_t power_pin_down(void) {
    return gpio_set_level(POWER_PIN, 0);
}


esp_err_t power_pin_up(void) {
    return gpio_set_level(POWER_PIN, 1);
}


esp_err_t set_bias(void) {
    dac_output_enable(DAC_CHANNEL);
    return dac_output_voltage(DAC_CHANNEL, BIAS_DAC_VALUE);
}


int adc1_channel_setup(uint32_t channel, esp_adc_cal_characteristics_t *chars) {
    int ret = 0;
    ret |= adc1_config_channel_atten(channel, ADC_ATTENUATION);
    ret |= esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTENUATION, ADC_WIDTH_BIT_12, ADC_VREF, chars);
    return ret;
}


int adcs_setup(void) {
    int ret = 0;
    adc1_config_width(ADC_WIDTH_BIT_12);

    for(int i = 0; i < N_ADC; i++)
        ret |= adc1_channel_setup(adc_params[i].channel, &adc_params[i].adc_chars);
    
    return ret;
}


int setup_adc_reader(){
    // allocate memory for send buffers
    for(int i = 0; i < N_ADC_MEASURES; i++)
        adcs_send_buffers[i].samples = malloc(sizeof(int) * adc_params[i].window_size);

    //power pin configuration
    if(power_pin_setup() != ESP_OK || power_pin_up() != ESP_OK) {
        ESP_LOGE(TAG, "Failed configuring power pin.");
        return 1;
    }

    // DAC configuration
    if(set_bias() != ESP_OK) {
        ESP_LOGE(TAG, "Failed configuring DAC.");
        return 1;
    }

    // ADCs configuration
    if(adcs_setup() != ESP_OK) {
        ESP_LOGE(TAG, "Failed configuring ADCs.");
        return 1;
    }

    // timers configuration
    for(int i = 0; i < N_ADC_MEASURES; i++) {

    
        // sampling adc timer
        esp_timer_create(&sample_timer_args[i], &sampling_timer[i]);
        esp_timer_start_periodic(sampling_timer[i], adc_params[i].sample_frequency*1000000);
    }

    // broker sender timer. Just use one callback and send all the data (irradiation and battery
    // in the same message.
    ESP_LOGD(TAG, "Inicialazing broker sender timer\n");
    esp_timer_create(&broker_sender_timer_args[0], &broker_sender_timer[0]);
    esp_timer_start_periodic(broker_sender_timer[0], adc_params[0].send_frenquency*1000000);

    return 0;
}


int start_timer(int adc, esp_timer_handle_t timer, int freq){
    esp_err_t ret = esp_timer_start_periodic(timer, freq*1000000);
    if (ret != ESP_OK){
        ESP_LOGE(TAG, "Error starting timer from ADC %d", adc);
        return 1;
    }
    return 0;
}


int start_broker_send_timers() {
    int ret = 0;
    
    for(int i = 0; i < N_ADC_MEASURES; i++)
        ret |= start_timer(i, broker_sender_timer[i], adc_params[i].send_frenquency);

    return ret;
}


int stop_timer(int adc, esp_timer_handle_t timer){
    esp_err_t ret = esp_timer_stop(timer);
    if (ret != ESP_OK){
        ESP_LOGE(TAG, "Error stopping timer from ADC %d", adc);
        return 1;
    }

    return 0;
}


int stop_broker_send_timers() {
    int ret = 0;
    for(int i = 0; i < N_ADC_MEASURES; i++)
        ret |= stop_timer(i, broker_sender_timer[i]);

    return ret;
}


int change_sample_frequency(int sample_freq, int adc){
    if (stop_timer(adc, sampling_timer[adc]))
        return 1;

    vTaskDelay(pdMS_TO_TICKS(500));

    adc_params[adc].sample_frequency = sample_freq;

    if (start_timer(adc, sampling_timer[adc], adc_params[adc].sample_frequency))
        return 1;

    ESP_LOGI(TAG, "Changed sample frequency to %d s in ADC %d", sample_freq, adc);
    return 0;
}


int change_broker_sender_frequency(int send_freq, int adc) {
    if (stop_timer(adc, broker_sender_timer[adc]))
        return 1;

    vTaskDelay(pdMS_TO_TICKS(500));

    adc_params[adc].send_frenquency = send_freq;

    if (start_timer(adc, broker_sender_timer[adc], adc_params[adc].send_frenquency))
        return 1;

    ESP_LOGI(TAG, "Changed broker send frequency to %d s in ADC %d", send_freq, adc);
    return 0;
}


int change_sample_number(int n_samples, int adc) {
    if (stop_timer(adc, sampling_timer[adc]))
        return 1;
    
    vTaskDelay(pdMS_TO_TICKS(500));

    adc_params[adc].n_samples = n_samples;

    if (start_timer(adc, sampling_timer[adc], adc_params[adc].sample_frequency))
        return 1;

    ESP_LOGI(TAG, "Changed sample number to %d in ADC %d", n_samples, adc);
    return 0;
}

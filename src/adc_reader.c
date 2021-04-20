#include "adc_reader.h"

static const char *TAG = "adc_reader";
extern void enviar_al_broker(const char *topic, const char *data, int len, int qos, int retain);

int read_adc(float *value, int adc_index) {
    
    return 0;
}


static void sampeling_timer_callback(void * args){
    int adc_index = (int *) args;

    struct adc_reading data, sample;

    for(int i= 0 ; i < adc_params[adc_index].n_samples; i++){
        if (read_adc(&data.value, adc_index)){
            ESP_LOGE(TAG, "Error reading ADC with index %d", adc_index);
        } else {
            sample.value = data.value;
        }
    }
    sample.value = sample.value / adc_params[adc_index].n_samples;

    ESP_LOGI(TAG, "Sample from ADC(%d) = %d", adc_index, sample.value);    
    
    //Save the taken sample in the circular buffer
    adcs_send_buffers[adc_index].samples[(adcs_send_buffers[adc_index].ini 
            + adcs_send_buffers[adc_index].cont) % adc_params[adc_index].window_size] = sample;

    if (adcs_send_buffers[adc_index].cont == adc_params[adc_index].window_size) {
        // If buffer is full, rewrite first sample
        adcs_send_buffers[adc_index].ini = adcs_send_buffers[adc_index].ini % adc_params[adc_index].window_size;
    } 
    else {
        adcs_send_buffers[adc_index].cont++;
    }
}


static void broker_sender_callback(void * args){
    int adc_index = (int *) args;

    //See if there are samples to send
    if (adcs_send_buffers[adc_index].cont > 0){
        struct adc_reading mean = { .value = 0 };

        // made the sample mean
        for (int i = 0; i < adcs_send_buffers[adc_index].cont; i++)
            mean.value += adcs_send_buffers[adc_index].samples[i].value;

        mean.value /= adcs_send_buffers[adc_index].cont;

        // Codify data with CBOR format and send it to the broker
        char str[50];
        sprintf(str, "%f", mean.value);
        ESP_LOGD(TAG, "Send it to the broker\n");
        enviar_al_broker(adc_params[adc_index].mqtt_topic, &str, 0, 1, 0);
    } 
    else {
        ESP_LOGW(TAG, "There are still not data to send\n");
    }
}

//TODO
void init_adcs(){
    // ADCs configuration

    // timers configuration
    for(int i = 0; i < N_ADC; i++) {
        // sampeling adc timer
        const esp_timer_create_args_t sample_timer_args = {
            .callback = &sampeling_timer_callback,
            .name = "sampeling_timer",
            .arg = (void *)i,
        }; 
        esp_timer_create(&sample_timer_args, &sampeling_timer[i]);
        esp_timer_start_periodic(sampeling_timer[i], adc_params[i].sample_frequency*1000000);

        // broker sender timer
        ESP_LOGD(TAG, "Inicialazing broker sender timer\n");
        const esp_timer_create_args_t broker_sender_timer_args = {
            .name = "broker_sender_timer",
            .arg = (void *)i,
        };
        esp_timer_create(&broker_sender_timer_args, &broker_sender_timer[i]);
        esp_timer_start_periodic(broker_sender_timer[i], adc_params[i].send_frenquency*1000000);
    }
}


int start_timer(int adc, esp_timer_handle_t timer, int freq){
    esp_err_t ret = esp_timer_start_periodic(timer, freq*1000000);
    if (ret != ESP_OK){
        ESP_LOGE(TAG, "Error starting timer from ADC %d", adc);
        return 1;
    }
    return 0;
}


int stop_timer(int adc, esp_timer_handle_t timer){
    esp_err_t ret = esp_timer_stop(timer);
    if (ret != ESP_OK){
        ESP_LOGE(TAG, "Error stopping timer from ADC %d", adc);
        return 1;
    }

    return 0;
}


int change_sample_frequency(int sample_freq, int adc){
    if (stop_timer(adc, sampeling_timer[adc]))
        return 1;

    vTaskDelay(pdMS_TO_TICKS(500));

    adc_params[adc].sample_frequency = sample_freq;

    if (start_timer(adc, sampeling_timer[adc], adc_params[adc].sample_frequency))
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
    if (stop_timer(adc, sampeling_timer[adc]))
        return 1;
    
    vTaskDelay(pdMS_TO_TICKS(500));

    adc_params[adc].n_samples = n_samples;

    if (start_timer(adc, sampeling_timer[adc], adc_params[adc].sample_frequency))
        return 1;

    ESP_LOGI(TAG, "Changed sample number to %d in ADC %d", n_samples, adc);
    return 0;
}

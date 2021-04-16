#include "si7021.h"

static const char *TAG = "si7021";
extern void enviar_al_brocker(const char *topic, const char *data, int len, int qos, int retain);

esp_err_t
_writeCommandBytes(const i2c_port_t i2c_num, const uint8_t *i2c_command,
                   const size_t nbytes)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);

    // write the 7-bit address of the sensor to the bus, using the last bit to
    // indicate we are performing a write.
    i2c_master_write_byte(cmd, SI7021_SENSOR_ADDR << 1 | I2C_MASTER_WRITE,
                          ACK_CHECK_EN);

    for (size_t i = 0; i < nbytes; i++)
        i2c_master_write_byte(cmd, i2c_command[i], ACK_CHECK_EN);

    // send all queued commands, blocking until all commands have been sent.
    // note that this is *not* thread-safe.
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(i2c_num, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);

    return ret;
}

esp_err_t
_readResponseBytes(const i2c_port_t i2c_num, uint8_t *output,
                   const size_t nbytes)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);

    // write the 7-bit address of the sensor to the queue, using the last bit
    // to indicate we are performing a read.
    i2c_master_write_byte(cmd, SI7021_SENSOR_ADDR << 1 | I2C_MASTER_READ,
                          ACK_CHECK_EN);

    // read nbytes number of bytes from the response into the buffer. make
    // sure we send a NACK with the final byte rather than an ACK.
    for (size_t i = 0; i < nbytes; i++)
    {
        i2c_master_read_byte(cmd, &output[i], i == nbytes - 1
                                              ? NACK_VAL
                                              : ACK_VAL);
    }

    // send all queued commands, blocking until all commands have been sent.
    // note that this is *not* thread-safe.
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(i2c_num, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);

    return ret;
}


esp_err_t
_getSensorReading(const i2c_port_t i2c_num, const uint8_t command,
                  float *output, float (*fn)(const uint16_t))
{
    esp_err_t ret = _writeCommandBytes(i2c_num, &command, 1);

    if (ret != ESP_OK)
        return ret;

    // delay for 100ms between write and read calls, as the sensor can take
    // some time to respond.
    vTaskDelay(100 / portTICK_RATE_MS);

    // all sensor readings return a 16-bit value for this sensor.
    uint8_t buf[2];
    ret = _readResponseBytes(i2c_num, buf, 2);

    if (ret != ESP_OK)
        return ret;

    // re-assemble the bytes, and call the specified code-conversion function
    // to finally retrieve our final sensor reading value, writing it out to
    // the output pointer.
    uint16_t bytes = buf[0] << 8 | buf[1];
    *output = fn(bytes);

    return ESP_OK;
}

float
_temp_code_to_celsius(const uint16_t temp_code)
{
    // refer to page 22 of the datasheet for more information.
    return (( (175.72 * temp_code) / 65536.0 ) - 46.85);
}

float
_rh_code_to_pct(const uint16_t rh_code)
{
    // refer to page 21 of the datasheet for more information.
    return (( (125.0 * rh_code) / 65536.0 ) - 6.0);
}

esp_err_t
readTemperature(const i2c_port_t i2c_num, float *temperature)
{
    esp_err_t ret = _getSensorReading(i2c_num, SI7021_READ_TEMP, temperature,
                                      &_temp_code_to_celsius);

    return ret;
}

esp_err_t
readHumidity(const i2c_port_t i2c_num, float *humidity)
{
    esp_err_t ret = _getSensorReading(i2c_num, SI7021_READ_RH, humidity,
                                      &_rh_code_to_pct);

    return ret;
}

esp_err_t
readTemperatureAfterHumidity(const i2c_port_t i2c_num, float *temperature)
{
    esp_err_t ret = _getSensorReading(i2c_num, SI7021_READ_TEMP_PREV_RH,
                                      temperature, &_temp_code_to_celsius);

    return ret;
}

esp_err_t
readSensors(const i2c_port_t i2c_num, struct si7021_reading *sensor_data)
{
    esp_err_t ret = readHumidity(i2c_num, &sensor_data->humidity);

    if (ret != ESP_OK)
        return ret;

    ret = readTemperatureAfterHumidity(i2c_num, &sensor_data->temperature);

    return ret;
}

/**
 * @brief i2c master initialization
 */
esp_err_t i2c_master_init(void)
{
    int i2c_master_port = I2C_MASTER_NUM;
    i2c_config_t conf;
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = I2C_MASTER_SDA_IO;
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.scl_io_num = I2C_MASTER_SCL_IO;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf.master.clk_speed = I2C_MASTER_FREQ_HZ;
    i2c_param_config(i2c_master_port, &conf);
    return i2c_driver_install(i2c_master_port, conf.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);
}


static void periodic_timer_callback_temp(void * args){   
    struct si7021_reading datos, muestra;

    for(int i= 0 ; i < N_SAMPLES_TEMP; i++){
        esp_err_t ret = readSensors(I2C_MASTER_NUM, &datos);
        if (ret != ESP_OK){
            ESP_LOGE(TAG, "Error al leer la temperatura y la humedad");
        } else {
            muestra.humidity += datos.humidity;
            muestra.temperature += datos.temperature;
        }
    }
    muestra.humidity = muestra.humidity / N_SAMPLES_TEMP;
    muestra.temperature = muestra.temperature / N_SAMPLES_TEMP;

    ESP_LOGI(TAG, "Muestra: %f ºC, %f humedad", muestra.temperature, muestra.humidity);    
    
    //Guardamos la muestra en el buffer circular
    listaMuestrasParaEnviarTemp.muestras[(listaMuestrasParaEnviarTemp.ini + listaMuestrasParaEnviarTemp.cont) % WINDOW_SIZE_TEMP] = muestra;
    if (listaMuestrasParaEnviarTemp.cont == WINDOW_SIZE_TEMP){
        //El buffer está lleno, he machacado el elemento inicial
        listaMuestrasParaEnviarTemp.ini = listaMuestrasParaEnviarTemp.ini % WINDOW_SIZE_TEMP;
    } else{
        listaMuestrasParaEnviarTemp.cont++;
    }

}

static void periodic_timer_callback_envio_al_brocker(void * args){
    //Veamos si hay elementos para enviar
    if (listaMuestrasParaEnviarTemp.cont > 0){
        struct si7021_reading media = {
            .humidity = 0,
            .temperature = 0,
        };

        //Hacemos la media de lo que hay
        for (int i=0; i < listaMuestrasParaEnviarTemp.cont; i++){
            media.humidity += listaMuestrasParaEnviarTemp.muestras[i].humidity;
            media.temperature += listaMuestrasParaEnviarTemp.muestras[i].temperature;
        }
        media.humidity /= listaMuestrasParaEnviarTemp.cont;
        media.temperature /= listaMuestrasParaEnviarTemp.cont;
        //Codificamos la informacion con CBOR y la enviamos al brocker
        char str_temp[50];
        sprintf(str_temp, "%f", media.temperature);
        ESP_LOGD(TAG, "Enviamos al brocker temp\n");
        enviar_al_brocker(MI_TOPIC_TEMP, &str_temp, 0, 1, 0);
        char str_hum[50];
        sprintf(str_hum, "%f", media.humidity);
        ESP_LOGD(TAG, "Enviamos al brocker hum\n");
        enviar_al_brocker(MI_TOPIC_HUM, &str_hum, 0, 1, 0);
    } else{
        ESP_LOGW(TAG, "No hay información para enviar al brocker\n");
    }
}


void inicializarTimerSensorTemperaturaYHumedad(){
    ESP_ERROR_CHECK(i2c_master_init());

    const esp_timer_create_args_t periodic_timer_args = {
        .callback = &periodic_timer_callback_temp,
        .name = "periodic"
    }; 
    esp_timer_create(&periodic_timer_args, &periodic_timer_sensor_temp_hum);
    esp_timer_start_periodic(periodic_timer_sensor_temp_hum, SAMPLE_FREQ_TEMP*1000000);

    //Iniciamos el timer de frecuencia de envío (> que el anterior)
    ESP_LOGD(TAG, "Iniciamos el timer de envío\n");
    const esp_timer_create_args_t periodic_timer_args_send = {
        .callback = &periodic_timer_callback_envio_al_brocker,
        .name = "periodic"
    }; 
    esp_timer_create(&periodic_timer_args_send, &periodic_timer_envio_al_brocker);
    esp_timer_start_periodic(periodic_timer_envio_al_brocker, SEND_FREQ_TEMP*1000000);
}

void activarTimerSensorTemp(){
    esp_timer_start_periodic(periodic_timer_envio_al_brocker, SEND_FREQ_TEMP*1000000);
}

void pararTimerSensorTemp(){
    esp_timer_stop(periodic_timer_envio_al_brocker);
}

struct si7021_reading getTempyHum(){
    struct si7021_reading datos, muestra = {.humidity = 0, .temperature = 0};

    for(int i= 0 ; i < N_SAMPLES_TEMP; i++){
        esp_err_t ret = readSensors(I2C_MASTER_NUM, &datos);
        if (ret != ESP_OK){
            ESP_LOGE(TAG, "Error al leer la temperatura y la humedad");
        } else {
            muestra.humidity += datos.humidity;
            muestra.temperature += datos.temperature;
        }
    }
    muestra.humidity = muestra.humidity / N_SAMPLES_TEMP;
    muestra.temperature = muestra.temperature / N_SAMPLES_TEMP;

    return muestra;
}

/*Modificamos los parámetros del sensor por peticiones POST o vía topic de MQTT*/
void modificaSampleFreqTemp(int sample_freq){
    esp_err_t ret = esp_timer_stop(periodic_timer_sensor_temp_hum);
    if (ret != ESP_OK){
        ESP_LOGE(TAG, "Error al stopear el timer");
    }
    vTaskDelay(pdMS_TO_TICKS(500));
    SAMPLE_FREQ_TEMP = sample_freq;
    ret = esp_timer_start_periodic(periodic_timer_sensor_temp_hum, SAMPLE_FREQ_TEMP*1000000);
    if (ret != ESP_OK){
        ESP_LOGE(TAG, "Error al iniciar el timer de nuevo");
    }
    ESP_LOGI(TAG, "Modificado el SAMPLE_FREQ %d", sample_freq);
}

void modificaSendFreqTemp(int send_freq){
    esp_err_t ret = esp_timer_stop(periodic_timer_envio_al_brocker);
    if (ret != ESP_OK){
        ESP_LOGE(TAG, "Error al stopear el timer");
    }
    vTaskDelay(pdMS_TO_TICKS(500));
    SEND_FREQ_TEMP = send_freq;
    ret = esp_timer_start_periodic(periodic_timer_envio_al_brocker, SEND_FREQ_TEMP*1000000);
    if (ret != ESP_OK){
        ESP_LOGE(TAG, "Error al stopear el timer");
    }
    ESP_LOGI(TAG, "Modificado el SEND_FREQ %d", send_freq);
}

void modificaNSamplesTemp(int n_samples){
    esp_timer_stop(periodic_timer_sensor_temp_hum);
    vTaskDelay(pdMS_TO_TICKS(500)); //Damos tiempo a que se termine de ejecutar el posible callback del timer donde se usa varias veces N_SAMPLES
    //TODO: Mejor un mutex (semaforo) compartido entre esta función y el callback
    N_SAMPLES_TEMP = n_samples;
    esp_timer_start_periodic(periodic_timer_sensor_temp_hum, SAMPLE_FREQ_TEMP*1000000);
}
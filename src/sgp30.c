#include <stdio.h>
#include "esp_log.h"
#include "driver/i2c.h"
#include "sdkconfig.h"

#include "sgp30.h"

extern void enviar_al_brocker(const char *topic, const char *data, int len, int qos, int retain);
extern int32_t getRestartCount();
extern int32_t getCO2BaselineFromNVM();
extern int32_t getTVOCBaselineFromNVM();
void setBaselineFromNVM(int32_t eco2_baseline, int32_t tvoc_baseline);

static uint8_t sgp30_calculate_CRC(uint8_t *data, uint8_t len) {
    /**
     ** Data and commands are protected with a CRC checksum to increase the communication reliability.
     ** The 16-bit commands that are sent to the sensor already include a 3-bit CRC checksum.
     ** Data sent from and received by the sensor is always succeeded by an 8-bit CRC.
     *! In write direction it is mandatory to transmit the checksum, since the SGP30 only accepts data if
     *! it is followed by the correct checksum. 
     *
     ** In read direction it is up to the master to decide if it wants to read and process the checksum
    */
    uint8_t crc = SGP30_CRC8_INIT;

    for (uint8_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (uint8_t b = 0; b < 8; b++) {
        if (crc & 0x80)
            crc = (crc << 1) ^ SGP30_CRC8_POLYNOMIAL;
        else
            crc <<= 1;
        }
    }
    return crc;
}


static esp_err_t sgp30_execute_command(sgp30_dev_t *device, uint8_t command[], uint8_t command_len, uint16_t delay, 
                                        uint16_t *read_data, uint8_t read_len) {

    /*********************************************************************************************
     ** Measurement routine: START condition, the I2C WRITE header (7-bit I2C device address plus 0
     ** as the write bit) and a 16-bit measurement command.
     **
     ** All commands are listed in TABLE 10 on the datasheet. 
     ** 
     ** 
     ** After the sensor has completed the measurement, the master can read the measurement results by 
     ** sending a START condition followed by an I2C READ header. The sensor will respond with the data.
     * 
     *! Each byte must be acknowledged by the microcontroller with an ACK condition for the sensor to continue sending data.
     *! If the sensor does not receive an ACK from the master after any byte of data, it will not continue sending data.
    **********************************************************************************************/

    esp_err_t err;

    // Writes SGP30 Command
    err = device->i2c_write(NULL_REG, command, command_len, device->intf_ptr);
    
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to write SGP30 I2C command! err: 0x%02x", err);
        return err;  
    }

    // Waits for device to process command and measure desired value
    vTaskDelay(delay / portTICK_RATE_MS);

    // Checks if there is data to be read from the user, (or if it's just a simple command write)
    if (read_len == 0) {
        return ESP_OK;
    }

    uint8_t reply_len = read_len * (SGP30_WORD_LEN + 1);
    uint8_t reply_buffer[reply_len];

    // Tries to read device reply
    err = device->i2c_read(NULL_REG, reply_buffer, reply_len, device->intf_ptr);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read SGP30 I2C command reply! err: 0x%02x", err);
        return err;  // failed to read reply buffer from chip
    }

    // Calculates expected CRC and compares it with the response
    for (uint8_t i = 0; i < read_len; i++) {
        uint8_t crc = sgp30_calculate_CRC(reply_buffer + i * 3, 2);
        ESP_LOGD(TAG, "%s - Calc CRC: %02x,   Reply CRC: %02x", __FUNCTION__, crc, reply_buffer[i * 3 + 2]);

        if (crc != reply_buffer[i * 3 + 2]) {
            ESP_LOGW(TAG, "Reply and Calculated CRCs are different");
            return false;
        }

        // If CRCs are equal, save data
        read_data[i] = reply_buffer[i * 3];
        read_data[i] <<= 8;
        read_data[i] |= reply_buffer[i * 3 + 1];
        ESP_LOGD(TAG, "%s - Read data: %04x", __FUNCTION__, read_data[i]);
    }

    return ESP_OK;
}


void sgp30_init(sgp30_dev_t *sensor, sgp30_read_fptr_t user_i2c_read, sgp30_write_fptr_t user_i2c_write) {
    sensor->intf_ptr = &SGP_DEVICE_ADDR; 
    sensor->i2c_read = user_i2c_read;
    sensor->i2c_write = user_i2c_write;

    sgp30_execute_command(sensor, GET_SERIAL_ID, 2, 10, sensor->serial_number, 3);
    ESP_LOGD(TAG, "%s - Serial Number: %02x %02x %02x", __FUNCTION__, sensor->serial_number[0],
                                sensor->serial_number[1], sensor->serial_number[2]);

    uint16_t featureset;
    sgp30_execute_command(sensor, GET_FEATURE_SET_VERSION, 2, 10, &featureset, 1);
    ESP_LOGD(TAG, "%s - Feature set version: %04x", __FUNCTION__, featureset);

    sgp30_IAQ_init(sensor);
}

void sgp30_softreset(sgp30_dev_t *sensor) {
    sgp30_execute_command(sensor, SOFT_RESET, 2, 10, NULL, 0);
}

void sgp30_IAQ_init(sgp30_dev_t *sensor) {
    sgp30_execute_command(sensor, INIT_AIR_QUALITY, 2, 10, NULL, 0);
}

void sgp30_IAQ_measure(sgp30_dev_t *sensor) {
    uint16_t reply[2];
    sgp30_execute_command(sensor, MEASURE_AIR_QUALITY, 2, 20, reply, 2);
    sensor->TVOC = reply[1];
    sensor->eCO2 = reply[0];
}

void sgp30_IAQ_measure_raw(sgp30_dev_t *sensor) {
    uint16_t reply[2];

    sgp30_execute_command(sensor, MEASURE_RAW_SIGNALS, 2, 20, reply, 2);
    sensor->raw_ethanol = reply[1];
    sensor->raw_H2 = reply[0];
}

void sgp30_get_IAQ_baseline(sgp30_dev_t *sensor, uint16_t *eco2_base, uint16_t *tvoc_base) {
    uint16_t reply[2];

    sgp30_execute_command(sensor, GET_BASELINE, 2, 20, reply, 2);

    *eco2_base = reply[0];
    *tvoc_base = reply[1];
}

void sgp30_set_IAQ_baseline(sgp30_dev_t *sensor, uint16_t eco2_base, uint16_t tvoc_base) {
    uint8_t baseline_command[8];

    baseline_command[0] = SET_BASELINE[0];
    baseline_command[1] = SET_BASELINE[1];

    baseline_command[2] = tvoc_base >> 8;
    baseline_command[3] = tvoc_base & 0xFF;
    baseline_command[4] = sgp30_calculate_CRC(baseline_command + 2, 2);

    baseline_command[5] = eco2_base >> 8;
    baseline_command[6] = eco2_base & 0xFF;
    baseline_command[7] = sgp30_calculate_CRC(baseline_command + 5, 2);

    sgp30_execute_command(sensor, baseline_command, 8, 20, NULL, 0);
}

void sgp30_set_humidity(sgp30_dev_t *sensor, uint32_t absolute_humidity) {
    if (absolute_humidity > 256000) {
        ESP_LOGW(TAG, "%s - Abs humidity value %d is too high!", __FUNCTION__, absolute_humidity);
        return;
    }

    uint8_t ah_command[5];
    uint16_t ah_scaled = (uint16_t)(((uint64_t)absolute_humidity * 256 * 16777) >> 24);

    ah_command[0] = SET_HUMIDITY[0];
    ah_command[1] = SET_HUMIDITY[1];

    ah_command[2] = ah_scaled >> 8;
    ah_command[3] = ah_scaled & 0xFF;
    ah_command[4] = sgp30_calculate_CRC(ah_command + 2, 2);

    sgp30_execute_command(sensor, ah_command, 5, 20, NULL, 0);
}





esp_err_t i2c_master_driver_initialize(void) {
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

/**
 * @brief generic function for reading I2C data
 * 
 * @param reg_addr register adress to read from 
 * @param reg_data pointer to save the data read 
 * @param len length of data to be read
 * @param intf_ptr 
 * 
 * >init: dev->intf_ptr = &dev_addr;
 * 
 * @return ESP_OK if reading was successful
 */
int8_t main_i2c_read(uint8_t reg_addr, uint8_t *reg_data, uint32_t len, void *intf_ptr) { // *intf_ptr = dev->intf_ptr
    int8_t ret = 0; /* Return 0 for Success, non-zero for failure */

    if (len == 0) {
        return ESP_OK;
    }

    uint8_t chip_addr = *(uint8_t*)intf_ptr;
    
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();

    i2c_master_start(cmd);
    
    if (reg_addr != 0xff) {
        i2c_master_write_byte(cmd, (chip_addr << 1) | I2C_MASTER_WRITE, ACK_CHECK_EN);
        i2c_master_write_byte(cmd, reg_addr, ACK_CHECK_EN);
        i2c_master_start(cmd);
    }
    
    i2c_master_write_byte(cmd, (chip_addr << 1) | I2C_MASTER_READ, ACK_CHECK_EN);

    if (len > 1) {
        i2c_master_read(cmd, reg_data, len - 1, ACK_VAL);
    }
    i2c_master_read_byte(cmd, reg_data + len - 1, NACK_VAL);
    i2c_master_stop(cmd);

    ret = i2c_master_cmd_begin(i2c_num, cmd, 1000 / portTICK_RATE_MS);
    
    i2c_cmd_link_delete(cmd);
    
    return ret;
}


/**
 * @brief generic function for writing data via I2C 
 *  
 * @param reg_addr register adress to write to 
 * @param reg_data register data to be written 
 * @param len length of data to be written
 * @param intf_ptr 
 * 
 * @return ESP_OK if writing was successful
 */
int8_t main_i2c_write(uint8_t reg_addr, uint8_t *reg_data, uint32_t len, void *intf_ptr) {
    int8_t ret = 0; /* Return 0 for Success, non-zero for failure */

    uint8_t chip_addr = *(uint8_t*)intf_ptr;

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (chip_addr << 1) | I2C_MASTER_WRITE, ACK_CHECK_EN);
    
    if (reg_addr != 0xFF) {
        i2c_master_write_byte(cmd, reg_addr, ACK_CHECK_EN);
    }

    i2c_master_write(cmd, reg_data, len, ACK_CHECK_EN);
    i2c_master_stop(cmd);

    ret = i2c_master_cmd_begin(i2c_num, cmd, 1000 / portTICK_RATE_MS);
    
    i2c_cmd_link_delete(cmd);
    
    return ret;
}


static void periodic_timer_callback_co2(void * args){
    sgp30_dev_t sgp30_sensor_muestra;
    sgp30_init(&sgp30_sensor_muestra, (sgp30_read_fptr_t)main_i2c_read, (sgp30_write_fptr_t)main_i2c_write);
    struct sgp30_reading muestra = {
        .TVOC = 0.0,
        .CO2 = 0.0
    };

    //Leemos N_SAMPLES veces el sensor para obtener una muestra
    for(int i= 0 ; i < N_SAMPLES_CO2; i++){
        vTaskDelay(1000 / portTICK_RATE_MS);
        sgp30_IAQ_measure(&sgp30_sensor_muestra);
        muestra.TVOC += sgp30_sensor_muestra.TVOC;
        muestra.CO2 += sgp30_sensor_muestra.eCO2;
    }
    muestra.TVOC = muestra.TVOC / (float) N_SAMPLES_CO2;
    muestra.CO2 = muestra.CO2 / (float) N_SAMPLES_CO2;

    ESP_LOGI(TAG, "Muestra: TVOC: %f,  eCO2: %f",  muestra.TVOC, muestra.CO2);
    //Guardamos la muestra en el buffer circular
    listaMuestrasParaEnviar.muestras[(listaMuestrasParaEnviar.ini + listaMuestrasParaEnviar.cont) % WINDOW_SIZE_CO2] = muestra;
    if (listaMuestrasParaEnviar.cont == WINDOW_SIZE_CO2){
        //El buffer está lleno, he machacado el elemento inicial
        listaMuestrasParaEnviar.ini = listaMuestrasParaEnviar.ini % WINDOW_SIZE_CO2;
    } else{
        listaMuestrasParaEnviar.cont++;
    }
}

static void periodic_timer_callback_envio_al_brocker(void * args){
    //Veamos si hay elementos para enviar
    if (listaMuestrasParaEnviar.cont > 0){
        struct sgp30_reading media = {
            .TVOC = 0,
            .CO2 = 0
        };
        //Hacemos la media de lo que hay
        for (int i=0; i < listaMuestrasParaEnviar.cont; i++){
            media.TVOC += listaMuestrasParaEnviar.muestras[i].TVOC;
            media.CO2 += listaMuestrasParaEnviar.muestras[i].CO2;
        }
        media.TVOC /= listaMuestrasParaEnviar.cont;
        media.CO2 /= listaMuestrasParaEnviar.cont;

        //Enviamos el dato de CO2 y TVOC al brocker
        char str_co2[50];
        sprintf(str_co2, "%f", media.CO2);
        ESP_LOGD(TAG, "Enviamos al brocker CO2\n");
        enviar_al_brocker(MI_TOPIC_CO2, &str_co2, 0, 1, 0);

        char str_tvoc[50];
        sprintf(str_tvoc, "%f", media.TVOC);
        ESP_LOGD(TAG, "Enviamos al brocker CO2\n");
        enviar_al_brocker(MI_TOPIC_TVOC, &str_tvoc, 0, 1, 0);
    } else{
        ESP_LOGW(TAG, "No hay información para enviar al brocker\n");
    }
}


void inicializarTimerSensorCO2(){

    ESP_ERROR_CHECK(i2c_master_driver_initialize());

    sgp30_dev_t main_sgp30_sensor;

    sgp30_init(&main_sgp30_sensor, (sgp30_read_fptr_t)main_i2c_read, (sgp30_write_fptr_t)main_i2c_write);

    uint16_t eco2_baseline = 0, tvoc_baseline = 0;
    // SGP30 needs to be read every 1s and sends TVOC = 400 14 times when initializing
    for (int i = 0; i < 14; i++) {
        vTaskDelay(1000 / portTICK_RATE_MS);
        sgp30_IAQ_measure(&main_sgp30_sensor);
        ESP_LOGI(TAG, "SGP30 Calibrating... TVOC: %d,  eCO2: %d",  main_sgp30_sensor.TVOC, main_sgp30_sensor.eCO2);
    }
    sgp30_get_IAQ_baseline(&main_sgp30_sensor, &eco2_baseline, &tvoc_baseline);
    ESP_LOGI(TAG, "BASELINES - TVOC: %d,  eCO2: %d",  tvoc_baseline, eco2_baseline);

    //Ahora iniciamos el timer DE RECOGIDA DE MUESTRAS
    ESP_LOGD(TAG, "Iniciamos el timer de muestreo\n");
    const esp_timer_create_args_t periodic_timer_args = {
        .callback = &periodic_timer_callback_co2,
        .name = "periodic"
    }; 
    esp_timer_create(&periodic_timer_args, &periodic_timer_sensor_co2);
    esp_timer_start_periodic(periodic_timer_sensor_co2, SAMPLE_FREQ_CO2*1000000);

    //Iniciamos el timer de frecuencia de envío (> que el anterior)
    ESP_LOGD(TAG, "Iniciamos el timer de envío\n");
    const esp_timer_create_args_t periodic_timer_args_send = {
        .callback = &periodic_timer_callback_envio_al_brocker,
        .name = "periodic"
    }; 
    esp_timer_create(&periodic_timer_args_send, &periodic_timer_envio_al_brocker);
    esp_timer_start_periodic(periodic_timer_envio_al_brocker, SEND_FREQ_CO2*1000000);
}

/*Activamos y desactivamos el timer de envio si se desconecta del brocker*/
void activarTimerSensorCO2(){
    esp_timer_start_periodic(periodic_timer_envio_al_brocker, SEND_FREQ_CO2*1000000);
}

void pararTimerSensorCO2(){
    esp_timer_stop(periodic_timer_envio_al_brocker);
}

/*Leemos el valor del CO2 para enviarlo a la petición GET*/
struct sgp30_reading get_co2(){
    sgp30_dev_t sgp30_sensor_muestra;
    sgp30_init(&sgp30_sensor_muestra, (sgp30_read_fptr_t)main_i2c_read, (sgp30_write_fptr_t)main_i2c_write);
    struct sgp30_reading muestra;

    //Leemos N_SAMPLES veces el sensor para obtener una muestra
    for(int i= 0 ; i < N_SAMPLES_CO2; i++){
        vTaskDelay(1000 / portTICK_RATE_MS);
        sgp30_IAQ_measure(&sgp30_sensor_muestra);
        muestra.TVOC += sgp30_sensor_muestra.TVOC;
        muestra.CO2 += sgp30_sensor_muestra.eCO2;
    }
    muestra.TVOC = muestra.TVOC / (float) N_SAMPLES_CO2;
    muestra.CO2 = muestra.CO2 / (float) N_SAMPLES_CO2;

    return muestra;
}

/*Modificamos los parámetros del sensor por peticiones POST o vía topic de MQTT*/
void modificaSampleFreqCO2(int sample_freq){
    esp_timer_stop(periodic_timer_sensor_co2);
    vTaskDelay(pdMS_TO_TICKS(500));
    esp_timer_start_periodic(periodic_timer_sensor_co2, sample_freq*1000000);
    SAMPLE_FREQ_CO2 = sample_freq;
    ESP_LOGI(TAG, "Modificado el SAMPLE_FREQ %d", sample_freq);
}

void modificaSendFreqCO2(int send_freq){
    esp_timer_stop(periodic_timer_envio_al_brocker);
    vTaskDelay(pdMS_TO_TICKS(500));
    esp_timer_start_periodic(periodic_timer_envio_al_brocker, send_freq*1000000);
    SEND_FREQ_CO2 = send_freq;
}

void modificaNSamplesCO2(int n_samples){
    esp_timer_stop(periodic_timer_sensor_co2);
    vTaskDelay(pdMS_TO_TICKS(500)); //Damos tiempo a que se termine de ejecutar el posible callback del timer donde se usa varias veces N_SAMPLES
    //TODO: Mejor un mutex (semaforo) compartido entre esta función y el callback
    N_SAMPLES_CO2 = n_samples;
    esp_timer_start_periodic(periodic_timer_sensor_co2, SAMPLE_FREQ_CO2*1000000);
}

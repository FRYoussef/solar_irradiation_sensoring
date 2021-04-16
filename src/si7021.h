#include <stdio.h>
#include "esp_log.h"
#include "driver/i2c.h"
#include "sdkconfig.h"
#include "mqtt_client.h"

#include "esp_wifi.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"

#include "freertos/FreeRTOS.h"
#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"
#include "/home/ubuntu/esp/esp-idf/examples/common_components/protocol_examples_common/include/protocol_examples_common.h"

#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "esp_sleep.h"
#include "esp_pm.h"

#include "sensors_readings.h"

#define _I2C_NUMBER(num) I2C_NUM_##num
#define I2C_NUMBER(num) _I2C_NUMBER(num)

#define DATA_LENGTH 512                  /*!< Data buffer length of test buffer */
#define RW_TEST_LENGTH 128               /*!< Data length for r/w test, [0,DATA_LENGTH] */
#define DELAY_TIME_BETWEEN_ITEMS_MS 1000 /*!< delay time between different test items */


#define I2C_MASTER_SCL_IO CONFIG_I2C_MASTER_SCL_TEMP               /*!< gpio number for I2C master clock */
#define I2C_MASTER_SDA_IO CONFIG_I2C_MASTER_SDA_TEMP               /*!< gpio number for I2C master data  */
#define I2C_MASTER_NUM I2C_NUMBER(CONFIG_I2C_MASTER_PORT_NUM_TEMP) /*!< I2C port number for master dev */
#define I2C_MASTER_FREQ_HZ CONFIG_I2C_MASTER_FREQUENCY_TEMP        /*!< I2C master clock frequency */
#define I2C_MASTER_TX_BUF_DISABLE 0                           /*!< I2C master doesn't need buffer */
#define I2C_MASTER_RX_BUF_DISABLE 0                           /*!< I2C master doesn't need buffer */


#define SI7021_READ_TEMP 0xF3
#define SI7021_READ_RH 0xF5
#define SI7021_READ_TEMP_PREV_RH 0xE0
#define SI7021_SENSOR_ADDR 0x40

#define WRITE_BIT I2C_MASTER_WRITE              /*!< I2C master write */
#define READ_BIT I2C_MASTER_READ                /*!< I2C master read */
#define ACK_CHECK_EN 0x1                        /*!< I2C master will check ack from slave*/
#define ACK_CHECK_DIS 0x0                       /*!< I2C master will not check ack from slave */
#define ACK_VAL 0x0                             /*!< I2C ack value */
#define NACK_VAL 0x1                            /*!< I2C nack value */

static const char * MI_TOPIC_TEMP = "/Informatica/1/9/TEMP/3"; // Facultad/Piso/Aula/TipoSensor/NumSensor
static const char * MI_TOPIC_HUM = "/Informatica/1/9/HUM/3";


#define WINDOW_SIZE_TEMP CONFIG_WINDOW_SIZE_TEMP
int SAMPLE_FREQ_TEMP = CONFIG_SAMPLE_FREQ_TEMP;
int SEND_FREQ_TEMP = CONFIG_SEND_FREQ_TEMP;
int N_SAMPLES_TEMP = CONFIG_N_SAMPLES_TEMP;


struct bufferMuestrasEnvio {
    struct si7021_reading muestras[WINDOW_SIZE_TEMP];
    int ini;
    int cont;
};

struct bufferMuestrasEnvio listaMuestrasParaEnviarTemp = {
        .ini = 0,
        .cont = 0
};

esp_timer_handle_t periodic_timer_envio_al_brocker;
esp_timer_handle_t periodic_timer_sensor_temp_hum;
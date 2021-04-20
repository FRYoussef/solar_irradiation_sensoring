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
//#include "/home/ubuntu/esp/esp-idf/examples/common_components/protocol_examples_common/include/protocol_examples_common.h"

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

static const char * TOPIC_TEMP = "/ciu/lopy4/irradiation/1"; // /location/board name/sensor metric/sensor number
static const char * TOPIC_HUM = "/ciu/lopy4/irradiation/1";


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
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

#define DATA_LENGTH 512                  /*!< Data buffer length of test buffer */
#define RW_TEST_LENGTH 128               /*!< Data length for r/w test, [0,DATA_LENGTH] */
#define DELAY_TIME_BETWEEN_ITEMS_MS 1000 /*!< delay time between different test items */

struct adc_config_params {
    int window_size;
    int sample_frequency;
    int send_frenquency;
    int n_samples;
    char *mqtt_topic;
};

// inizialise for each adc its parameters
static struct adc_config_params adc_params[N_ADC] = {
    {
        .window_size = CONFIG_WINDOW_SIZE_IRRAD,
        .sample_frequency = CONFIG_SAMPLE_FREQ_IRRAD,
        .send_frenquency = CONFIG_SEND_FREQ_IRRAD,
        .n_samples = CONFIG_N_SAMPLES_IRRAD,
        .mqtt_topic = CONFIG_MQTT_TOPIC_IRRAD,
    },
    {
        .window_size = CONFIG_WINDOW_SIZE_BATTERY,
        .sample_frequency = CONFIG_SAMPLE_FREQ_BATTERY,
        .send_frenquency = CONFIG_SEND_FREQ_BATTERY,
        .n_samples = CONFIG_N_SAMPLES_BATTERY,
        .mqtt_topic = CONFIG_MQTT_TOPIC_BATTERY,
    },
};

struct send_sample_buffer {
    struct adc_reading *samples;
    int ini;
    int cont;
};

struct send_sample_buffer adcs_send_buffers[N_ADC] = {
    {
        .samples = { [0 ... adc_params[IRRADIATION_ADC_INDEX].window_size-1] = 0 },
        .ini = 0,
        .cont = 0,
    },
    {
        .samples = { [0 ... adc_params[BATTERY_ADC_INDEX].window_size-1] = 0 },
        .ini = 0,
        .cont = 0,
    },
};

esp_timer_handle_t sampeling_timer[N_ADC];
esp_timer_handle_t broker_sender_timer[N_ADC];

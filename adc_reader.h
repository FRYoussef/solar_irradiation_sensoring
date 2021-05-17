#include <stdio.h>
#include "esp_log.h"
#include "sdkconfig.h"
#include "mqtt_client.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include <driver/adc.h>
#include <driver/dac.h>
#include <esp_adc_cal.h>
#include <string.h>

// define number of ADCs to read, and its indices
#define N_ADC 3 // ADC channels used
#define N_ADC_MEASURES 2 // number of ADCs used for own measures (different
                        //channels could be used to calculate the same measure)

#define POWER_PIN 21  // GPIO 21, P12 from LoPy4

/* BIAS for the measuring circuit
 * bias = vdd * dac_value / 255 
 * computation in mv to avoid floating point
 */
#define VDD  3300 // mv
#define BIAS 500  // mv
#define BIAS_DAC_VALUE (((BIAS * 255) + VDD/2)/ VDD)
#define DAC_CHANNEL DAC_CHANNEL_1

#define ADC_VREF 1100
#define ADC_ATTENUATION ADC_ATTEN_DB_11

// INFLUXDB Format 
#define MAX_INFLUXDB_FIELDS 4
#define MAX_INFLUXDB_STRING 128
#define INFLUXDB_MEASUREMENT "cabahla"
#define INFLUXDB_LOCATION ",id=v1-n3"
#define FIELD_IRRADIATION "irradiation"
#define FIELD_BATTERY "battery"


// MQTT topics
#define TOPIC_IRRADIATION "/ciu/lopy4/irradiation/1"
#define TOPIC_BATTERY_LEVEL "/ciu/lopy4/battery_level/1"
#define MQTT_TOPIC "/ciu/lopy4/irradiation/1"

int get_adc_mv(int *value, int adc_index);
int get_irradiation_mv(int *value, int adc_index);
int get_battery_mv(int *value, int adc_index);
static void sampling_timer_callback(void *);
static void broker_sender_callback(void *);

struct adc_config_params {
    int window_size;
    int sample_frequency;
    int send_frenquency;
    int n_samples;
    int channel;
    char *influxdb_field;
    esp_adc_cal_characteristics_t adc_chars;
    int (*get_mv)(int *, int);
    int last_mean;
};

struct send_sample_buffer {
    int ini;
    int cont;
    int *samples;
    char payload[20];
};

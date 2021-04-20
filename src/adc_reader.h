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

#include "sensors_readings.h"

/* BIAS for the measuring circuit
 * bias = vdd * dac_value / 255 
 * computation in mv to avoid floating point
 */
#define VDD  3300 // mv
#define BIAS 500  // mv
#define BIAS_DAC_VALUE (((BIAS * 255) + VDD/2)/ VDD)

#define ADC_VREF 1100
#define ADC_ATTENUATION ADC_ATTEN_DB_11

struct adc_config_params {
    int window_size;
    int sample_frequency;
    int send_frenquency;
    int n_samples;
    int power_pin;
    char *mqtt_topic;
};

// inizialise for each adc its parameters
static struct adc_config_params adc_params[N_ADC] = {
    {
        .window_size = CONFIG_WINDOW_SIZE_IRRAD,
        .sample_frequency = CONFIG_SAMPLE_FREQ_IRRAD,
        .send_frenquency = CONFIG_SEND_FREQ_IRRAD,
        .n_samples = CONFIG_N_SAMPLES_IRRAD,
        .power_pin = 21, // GPIO 21, P12 from LoPy 
        .mqtt_topic = CONFIG_MQTT_TOPIC_IRRAD,
    },
    {
        .window_size = CONFIG_WINDOW_SIZE_BATTERY,
        .sample_frequency = CONFIG_SAMPLE_FREQ_BATTERY,
        .send_frenquency = CONFIG_SEND_FREQ_BATTERY,
        .n_samples = CONFIG_N_SAMPLES_BATTERY,
        .power_pin = 0, //TO-CHANGE
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

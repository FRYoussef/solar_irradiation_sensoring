#include <stdio.h>
#include "mqtt_client.h"
#include "esp_log.h"
#include "sensors_readings.h"

static esp_mqtt_client_handle_t client;
#define BROKER_URI CONFIG_BROKER_URL


bool first_conexion = true;

// /location/board name/sensor metric/sensor number/config parameter
const char *MQTT_TOPIC_IRRAD = CONFIG_MQTT_TOPIC_IRRAD;
const char *MQTT_TOPIC_BATTERY = CONFIG_MQTT_TOPIC_BATTERY;

static const char * TOPIC_SAMPLE_FREQ_IRRADIATION;
sprintf(TOPIC_SAMPLE_FREQ_IRRADIATION, "%s/sample_frequency", MQTT_TOPIC_IRRAD);

static const char * TOPIC_SEND_FREQ_IRRADIATION;
sprintf(TOPIC_SEND_FREQ_IRRADIATION, "%s/send_frequency", MQTT_TOPIC_IRRAD);

static const char * TOPIC_N_SAMPLES_IRRADIATION;
sprintf(TOPIC_N_SAMPLES_IRRADIATION, "%s/sample_number", MQTT_TOPIC_IRRAD);

static char * TOPIC_SAMPLE_FREQ_BATTERY_LEVEL;
sprintf(TOPIC_SAMPLE_FREQ_BATTERY_LEVEL, "%s/sample_frequency", MQTT_TOPIC_BATTERY);

static char * TOPIC_SEND_FREQ_BATTERY_LEVEL;
sprintf(TOPIC_SEND_FREQ_BATTERY_LEVEL, "%s/send_frequency", MQTT_TOPIC_BATTERY);

static char * TOPIC_N_SAMPLES_BATTERY_LEVEL;
sprintf(TOPIC_N_SAMPLES_BATTERY_LEVEL, "%s/sample_number", MQTT_TOPIC_BATTERY);
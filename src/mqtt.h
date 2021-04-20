#include "mqtt_client.h"
#include "esp_log.h"
#include "sensors_readings.h"

static esp_mqtt_client_handle_t client;
#define BROKER_URI CONFIG_BROKER_URL


bool first_conexion = true;
                                                    // /location/board name/sensor metric/sensor number/config parameter  
static const char * TOPIC_SAMPLE_FREQ_IRRADIATION = "/ciu/lopy4/irradiation/1/sample_frequency";
static const char * TOPIC_SEND_FREQ_IRRADIATION = "/ciu/lopy4/irradiation/1/send_frequency";
static const char * TOPIC_N_SAMPLES_IRRADIATION = "/ciu/lopy4/irradiation/1/sample_number";

static const char * TOPIC_SAMPLE_FREQ_BATTERY_LEVEL = "/ciu/lopy4/battery_level/1/sample_frequency";
static const char * TOPIC_SEND_FREQ_BATTERY_LEVEL = "/ciu/lopy4/battery_level/1/send_frequency";
static const char * TOPIC_N_SAMPLES_BATTERY_LEVEL = "/ciu/lopy4/battery_level/1/sample_number";
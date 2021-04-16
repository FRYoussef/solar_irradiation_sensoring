#include "mqtt_client.h"
#include "esp_log.h"

static esp_mqtt_client_handle_t client;
#define BROKER_URI CONFIG_BROKER_URL


bool primeraConexion = true;

static const char * TOPIC_SAMPLE_FREQ_IRRADIATION = "/lopy4/irradiation/sample_frequency";
static const char * TOPIC_SEND_FREQ_IRRADIATION = "/lopy4/irradiation/send_frequency";
static const char * TOPIC_N_SAMPLES_IRRADIATION = "/lopy4/irradiation/sample_number";

static const char * TOPIC_SAMPLE_FREQ_BATTERY_LEVEL = "/lopy4/battery_level/sample_frequency";
static const char * TOPIC_SEND_FREQ_BATTERY_LEVEL = "/lopy4/battery_level/send_frequency";
static const char * TOPIC_N_SAMPLES_BATTERY_LEVEL = "/lopy4/battery_level/sample_number";
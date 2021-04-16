#include "mqtt_client.h"
#include "esp_log.h"

static esp_mqtt_client_handle_t client;
#define BROKER_URI CONFIG_BROKER_URL


bool primeraConexion = true;

static const char * TOPIC_SAMPLE_FREQ_CO2 = "/Informatica/1/9/CO2/3/SAMPLE_FREQ";
static const char * TOPIC_SEND_FREQ_CO2 = "/Informatica/1/9/CO2/3/SEND_FREQ";
static const char * TOPIC_N_SAMPLES_CO2 = "/Informatica/1/9/CO2/3/N_SAMPLES";
static const char * TOPIC_SAMPLE_FREQ_TEMP = "/Informatica/1/9/TEMP/3/SAMPLE_FREQ";
static const char * TOPIC_SEND_FREQ_TEMP = "/Informatica/1/9/TEMP/3/SEND_FREQ";
static const char * TOPIC_N_SAMPLES_TEMP = "/Informatica/1/9/TEMP/3/N_SAMPLES";
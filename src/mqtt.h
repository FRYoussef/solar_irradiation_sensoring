#include "mqtt_client.h"
#include "esp_log.h"

static esp_mqtt_client_handle_t client;
#define BROKER_URI CONFIG_BROKER_URL


bool first_conexion_mqtt = true;
#include "mqtt_client.h"
#include "esp_log.h"

#define BROKER_URI CONFIG_BROKER_URL

void mqtt_send_data(const char *topic, char *data, int len, int qos, int retain);
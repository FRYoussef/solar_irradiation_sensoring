/* MQTT over SSL Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include "mqtt.h"
#include "fsm.h"
#include "adc_reader.h"

static esp_mqtt_client_handle_t client;
static const char *TAG = "MQTTS";

// /location/board name/sensor metric/sensor number/config parameter
static const char * TOPIC_SAMPLE_FREQ_IRRADIATION = "/ciu/lopy4/irradiation/1/sample_frequency";
static const char * TOPIC_SEND_FREQ_IRRADIATION = "/ciu/lopy4/irradiation/1/send_frequency";
static const char * TOPIC_N_SAMPLES_IRRADIATION = "/ciu/lopy4/irradiation/1/sample_number";

static const char * TOPIC_SAMPLE_FREQ_BATTERY_LEVEL = "/ciu/lopy4/battery_level/1/sample_frequency";
static const char * TOPIC_SEND_FREQ_BATTERY_LEVEL = "/ciu/lopy4/battery_level/1/send_frequency";
static const char * TOPIC_N_SAMPLES_BATTERY_LEVEL = "/ciu/lopy4/battery_level/1/sample_number";


// #if CONFIG_BROKER_CERTIFICATE_OVERRIDDEN == 1
// static const uint8_t mqtt_eclipse_org_pem_start[]  = "-----BEGIN CERTIFICATE-----\n" CONFIG_BROKER_CERTIFICATE_OVERRIDE "\n-----END CERTIFICATE-----";
// #else
// extern const uint8_t mqtt_eclipse_org_pem_start[]   asm("_binary_mqtt_eclipse_org_pem_start");
// #endif
// extern const uint8_t mqtt_eclipse_org_pem_end[]   asm("_binary_mqtt_eclipse_org_pem_end");

static esp_err_t mqtt_event_handler_cb(esp_mqtt_event_handle_t event)
{
    client = event->client;
    // your_context_t *context = event->context;
    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
			esp_mqtt_client_subscribe(client, TOPIC_SAMPLE_FREQ_IRRADIATION, 1);
			esp_mqtt_client_subscribe(client, TOPIC_SEND_FREQ_IRRADIATION, 1);
			esp_mqtt_client_subscribe(client, TOPIC_N_SAMPLES_IRRADIATION, 1);
			esp_mqtt_client_subscribe(client, TOPIC_SAMPLE_FREQ_BATTERY_LEVEL, 1);
			esp_mqtt_client_subscribe(client, TOPIC_SEND_FREQ_BATTERY_LEVEL, 1);
			esp_mqtt_client_subscribe(client, TOPIC_N_SAMPLES_BATTERY_LEVEL, 1);
			fsm_mqtt_connected();
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
            break;

        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "MQTT_EVENT_DATA");
            event->data[event->data_len] = '\0';
			adc_reader_update(event->topic, event->data);
            //printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
            //printf("DATA=%.*s\r\n", event->data_len, event->data);

            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
            //printf("DATA=%.*s\r\n", event->data_len, event->data);
            break;
        default:
            ESP_LOGI(TAG, "Other event id:%d", event->event_id);
            break;
    }
    return ESP_OK;
}


static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%d", base, event_id);
    mqtt_event_handler_cb(event_data);
}


void mqtt_app_start(void)
{
    esp_mqtt_client_config_t mqtt_cfg = {
        .uri = BROKER_URI, //BROKER_URL
        //.uri =  "mqtt://urbion.dacya.ucm.es",
        //.cert_pem = (const char *)mqtt_eclipse_org_pem_start,
    };

    client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, client);
    esp_mqtt_client_start(client);
}


void mqtt_send_data(const char *topic, char *data, int len, int qos, int retain)
{
    esp_mqtt_client_publish(client, topic, data, len, qos, retain);
}

    

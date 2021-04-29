/* MQTT over SSL Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include "mqtt.h"

static esp_mqtt_client_handle_t client;
bool first_conexion_mqtt = true;

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

extern int setup_adc_reader();
extern int start_broker_send_timers();
extern int stop_broker_send_timers();
extern int change_sample_frequency(int sample_freq, int adc);
extern int change_broker_sender_frequency(int send_freq, int adc);
extern int change_sample_number(int n_samples, int adc);

extern int IRRADIATION_ADC_INDEX;
extern int BATTERY_ADC_INDEX;
extern int BIAS_ADC_INDEX;

static esp_err_t mqtt_event_handler_cb(esp_mqtt_event_handle_t event)
{
    client = event->client;
    // your_context_t *context = event->context;
    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");

            if (first_conexion_mqtt){
                /*Iniciamos los timers de lectura y envio*/
                if(setup_adc_reader()) {
                    ESP_LOGE(TAG, "Failed to create adc_reader module.");
                    return 1;
                }

                first_conexion_mqtt = false;

                esp_mqtt_client_subscribe(client, TOPIC_SAMPLE_FREQ_IRRADIATION, 1);
                esp_mqtt_client_subscribe(client, TOPIC_SEND_FREQ_IRRADIATION, 1);
                esp_mqtt_client_subscribe(client, TOPIC_N_SAMPLES_IRRADIATION, 1);

                esp_mqtt_client_subscribe(client, TOPIC_SAMPLE_FREQ_BATTERY_LEVEL, 1);
                esp_mqtt_client_subscribe(client, TOPIC_SEND_FREQ_BATTERY_LEVEL, 1);
                esp_mqtt_client_subscribe(client, TOPIC_N_SAMPLES_BATTERY_LEVEL, 1);
            } else {
                /*Activamos los timers de envio de los sensores*/
                start_broker_send_timers();
            }
            

            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
            /*Paramos los envios de los datos de los sensores*/
            stop_broker_send_timers();
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
            event->data[event->data_len] = '\0'; //Necesario para que no sea el buffer más grande y añada 0 que no queremos al usar atoi()
            if (strstr(event->topic, "sample_frequency") != NULL) {
                if (strstr(event->topic, "irradiation")){
                    change_sample_frequency(atoi(event->data), IRRADIATION_ADC_INDEX);
                    ESP_LOGI(TAG, "Recibido un cambio de sample_frequency a %d segundos para irradiation.", atoi(event->data));
                } else if (strstr(event->topic, "battery_level")){
                    change_sample_frequency(atoi(event->data), BATTERY_ADC_INDEX);
                    ESP_LOGI(TAG, "Recibido un cambio de sample_frequency a %d segundos para battery_level.", atoi(event->data));
                }
            } else if (strstr(event->topic, "send_frequency")){
                if (strstr(event->topic, "irradiation")){
                    change_broker_sender_frequency(atoi(event->data), IRRADIATION_ADC_INDEX);
                    ESP_LOGI(TAG, "Recibido un cambio de send_frequency a %d para irradiation.", atoi(event->data));
                } else if (strstr(event->topic, "battery_level")){
                    change_broker_sender_frequency(atoi(event->data), BATTERY_ADC_INDEX);
                    ESP_LOGI(TAG, "Recibido un cambio de send_frequency a %d para battery_level.", atoi(event->data));
                }
            } else if (strstr(event->topic, "sample_number")){
                if (strstr(event->topic, "irradiation")){
                    change_sample_number(atoi(event->data), IRRADIATION_ADC_INDEX);
                    ESP_LOGI(TAG, "Recibido un cambio de sample_number a %d para irradiation.", atoi(event->data));
                } else if (strstr(event->topic, "battery_level")){
                    change_sample_number(atoi(event->data), BATTERY_ADC_INDEX);
                    ESP_LOGI(TAG, "Recibido un cambio de sample_number a %d para battery_level.", atoi(event->data));
                }
            }
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
        //.uri = BROKER_URI, //BROKER_URL
        .uri =  "mqtt://urbion.dacya.ucm.es",
        //.cert_pem = (const char *)mqtt_eclipse_org_pem_start,
    };

    client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, client);
    esp_mqtt_client_start(client);
}


void mqtt_send_data(const char *topic, char *data, int len, int qos, int retain){
    esp_mqtt_client_publish(client, topic, data, len, qos, retain);
}

    
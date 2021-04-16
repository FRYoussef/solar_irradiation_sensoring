#include <math.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include "nvs.h"
#include "nvs_flash.h"

/*
#include "/home/ubuntu/esp/esp-idf/components/bt/include/esp_bt.h"
#include "/home/ubuntu/esp/esp-idf/components/bt/host/bluedroid/api/include/api/esp_gap_ble_api.h"
#include "/home/ubuntu/esp/esp-idf/components/bt/host/bluedroid/api/include/api/esp_gattc_api.h"
#include "/home/ubuntu/esp/esp-idf/components/bt/host/bluedroid/api/include/api/esp_gatt_defs.h"
#include "/home/ubuntu/esp/esp-idf/components/bt/host/bluedroid/api/include/api/esp_bt_main.h"
#include "/home/ubuntu/esp/esp-idf/components/bt/host/bluedroid/api/include/api/esp_gatt_common_api.h"*/

#include "esp_bt.h"
#include "esp_gap_ble_api.h"
#include "esp_gattc_api.h"
#include "esp_gatt_defs.h"
#include "esp_bt_main.h"
#include "esp_gatt_common_api.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include <time.h>
#include "freertos/task.h"


#define GATTC_TAG "GATTC_DEMO"
#define TAG "BLE"
#define REMOTE_SERVICE_UUID        0x00FF
#define REMOTE_NOTIFY_CHAR_UUID    0xFF01
#define PROFILE_NUM      1
#define PROFILE_A_APP_ID 0
#define INVALID_HANDLE   0

static const char remote_device_name[] = "ESP_GATTS_DEMO";
static bool connect    = false;
static bool get_server = false;
static esp_gattc_char_elem_t *char_elem_result   = NULL;
static esp_gattc_descr_elem_t *descr_elem_result = NULL;

/* Declare static functions */
static void esp_gap_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param);
static void esp_gattc_cb(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param);
static void gattc_profile_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param);


static esp_bt_uuid_t remote_filter_service_uuid = {
    .len = ESP_UUID_LEN_16,
    .uuid = {.uuid16 = REMOTE_SERVICE_UUID,},
};

static esp_bt_uuid_t remote_filter_char_uuid = {
    .len = ESP_UUID_LEN_16,
    .uuid = {.uuid16 = REMOTE_NOTIFY_CHAR_UUID,},
};

static esp_bt_uuid_t notify_descr_uuid = {
    .len = ESP_UUID_LEN_16,
    .uuid = {.uuid16 = ESP_GATT_UUID_CHAR_CLIENT_CONFIG,},
};

static esp_ble_scan_params_t ble_scan_params = {
    .scan_type              = BLE_SCAN_TYPE_ACTIVE,
    .own_addr_type          = BLE_ADDR_TYPE_PUBLIC,
    .scan_filter_policy     = BLE_SCAN_FILTER_ALLOW_ALL,
    .scan_interval          = 0x50,
    .scan_window            = 0x30,
    .scan_duplicate         = BLE_SCAN_DUPLICATE_DISABLE
};

typedef struct info_dispositivos_ble{
    esp_bd_addr_t addr;
    int8_t rssi;
}info_dispositivos_ble_t;

struct gattc_profile_inst {
    esp_gattc_cb_t gattc_cb;
    uint16_t gattc_if;
    uint16_t app_id;
    uint16_t conn_id;
    uint16_t service_start_handle;
    uint16_t service_end_handle;
    uint16_t char_handle;
    esp_bd_addr_t remote_bda;
};

#define AFORO_MAXIMO 50

info_dispositivos_ble_t tabla_dispositivos_ble[AFORO_MAXIMO];
int contadorTabla;

int DISTANCIA_MAXIMA = CONFIG_DISTANCIA_MAXIMA;
int TIME_SCAN = CONFIG_TIME_SCAN;
int FREQ_SCAN = CONFIG_FREQ_SCAN;

static const char * MI_TOPIC_AFORO = "/Informatica/1/9/AFORO";

esp_timer_handle_t periodic_timer_escaneo;
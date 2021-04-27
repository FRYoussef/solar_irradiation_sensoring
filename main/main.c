#include <stdio.h>
#include "esp_log.h"
#include "sdkconfig.h"
#include "mqtt_client.h"

#include "esp_wifi.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"

#include "freertos/FreeRTOS.h"

#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "esp_sleep.h"
#include "esp_pm.h"

#include "freertos/task.h"

extern void initialise_wifi(void);
extern void redireccionaLogs(void);

static const char *TAG = "main";

void app_main(void)
{   
    //Just redirect error logs
    esp_log_level_set("*", ESP_LOG_ERROR);
    redireccionaLogs();

    // Configuramos el gestor de energia
    esp_pm_config_esp32_t config = {
        .max_freq_mhz = CONFIG_MAX_CPU_FREQ_MHZ,
        .min_freq_mhz = CONFIG_MIN_CPU_FREQ_MHZ,
        .light_sleep_enable = true
    };
    esp_pm_configure(&config);

    /* Initialize networking stack */
    ESP_ERROR_CHECK(esp_netif_init());

    /* Create default event loop needed by the
     * main app and the provisioning service */
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    /* Initialize NVS needed by Wi-Fi */
    ESP_ERROR_CHECK(nvs_flash_init());

    ESP_LOGI(TAG, "Starting WiFi for WPA2 enterprise conexion");
    initialise_wifi();
    
    vTaskSuspend(NULL);
}
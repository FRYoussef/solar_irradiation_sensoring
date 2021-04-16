#include <stdio.h>
#include "esp_log.h"
#include "driver/i2c.h"
#include "sdkconfig.h"
#include "mqtt_client.h"

#include "esp_wifi.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"

#include "freertos/FreeRTOS.h"
#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"
//#include "/home/ubuntu/esp/esp-idf/examples/common_components/protocol_examples_common/include/protocol_examples_common.h"

#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "esp_sleep.h"
#include "esp_pm.h"

#include "freertos/task.h"

extern void provisioning(void);
extern void redireccionaLogs(void);

static const char *TAG = "main";

void app_main(void)
{   
    //Montamos una partición spiflash y redireccionamos los logs allí
    //esp_log_level_set("*", ESP_LOG_ERROR);
    //redireccionaLogs();

    // Configuramos el gestor de energia
    esp_pm_config_esp32_t config = {
        .max_freq_mhz = 240 ,
        .min_freq_mhz = 40 ,
        .light_sleep_enable = true
    };
    esp_pm_configure(& config );


    /* Initialize networking stack */
    ESP_ERROR_CHECK(esp_netif_init());

    /* Create default event loop needed by the
     * main app and the provisioning service */
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    /* Initialize NVS needed by Wi-Fi */
    ESP_ERROR_CHECK(nvs_flash_init());

    //Conectamos al WIFI
    //ESP_ERROR_CHECK(example_connect());

    //wifi provisioning
    ESP_LOGI(TAG, "Starting WiFi SoftAP provisioning");
    provisioning();
    
    
    vTaskSuspend(NULL);
}
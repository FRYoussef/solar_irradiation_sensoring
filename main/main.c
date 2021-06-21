#include <stdio.h>
#include "esp_log.h"
#include "sdkconfig.h"
#include "mqtt_client.h"
#include "fsm.h"

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
#include "energy-prof.h"

#include "freertos/task.h"

extern void redireccionaLogs(void);

void app_main(void)
{
    //Just redirect error logs
    esp_log_level_set("*", ESP_LOG_ERROR);
    //esp_log_level_set("*", ESP_LOG_VERBOSE);
    //redireccionaLogs();

    // Configuramos el gestor de energia
    esp_pm_config_esp32_t config = {
        .max_freq_mhz = CONFIG_MAX_CPU_FREQ_MHZ,
        .min_freq_mhz = CONFIG_MIN_CPU_FREQ_MHZ,
        .light_sleep_enable = EP_LIGHT_SLEEP_ENABLE,
    };
    esp_pm_configure(&config);

#ifdef EP_FULL_DEEP_SLEEP
	esp_sleep_enable_timer_wakeup((uint64_t)5*60*1000000LL);
	esp_deep_sleep_start();
#else
	fsm_init();
#endif
    vTaskSuspend(NULL);
}

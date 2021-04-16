/* LwIP SNTP example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_attr.h"
#include "esp_sleep.h"
#include "nvs_flash.h"
#include "esp_sntp.h"


static int32_t HOUR_TO_SLEEP = CONFIG_HOUR_TO_SLEEP;
static int32_t HOUR_TO_WAKEUP = CONFIG_HOUR_TO_WAKEUP;

static const char *TAG = "sntp";
esp_timer_handle_t deep_sleep_timer;

static void obtain_time(void);
static void initialize_sntp(void);

#ifdef CONFIG_SNTP_TIME_SYNC_METHOD_CUSTOM
void sntp_sync_time(struct timeval *tv)
{
   settimeofday(tv, NULL);
   ESP_LOGI(TAG, "Time is synchronized from custom code");
   sntp_set_sync_status(SNTP_SYNC_STATUS_COMPLETED);
}
#endif

struct tiempo {
    int horas;
    int minutos;
};

struct tiempo tiempoHastaDormir(struct tm timeinfo){
    struct tiempo tiempo;
    tiempo.horas = 0;
    tiempo.minutos = 60 - timeinfo.tm_min;
    if (timeinfo.tm_hour < HOUR_TO_SLEEP){
        //Aún duermo hoy
        tiempo.horas = HOUR_TO_SLEEP - timeinfo.tm_hour - 1;
    } else {
        //Duermo mañana
        tiempo.horas = 24 - timeinfo.tm_hour + HOUR_TO_SLEEP - 1;
    }

    return tiempo;
}


void modificaSleepHour(int sleep){
    HOUR_TO_SLEEP = sleep;
}

void modificaWakeupHour(int wakeup){
    HOUR_TO_WAKEUP = wakeup;
}

void updateDeepSleepTimer(){
    esp_timer_stop(deep_sleep_timer);
    time_t now;
    struct tm timeinfo;
    time(&now);
    setenv("TZ", "Europe/Madrid", 1);
    tzset();
    localtime_r(&now, &timeinfo);
    struct tiempo tiempo  = tiempoHastaDormir(timeinfo);
    long long int us_hasta_dormir = 1000000*60*(tiempo.minutos + 60*tiempo.horas);
    ESP_LOGI(TAG, "Cambio: hora de dormir = %d, hora de despertar = %d", HOUR_TO_SLEEP, HOUR_TO_WAKEUP);
    ESP_LOGI(TAG, "Iniciamos el timer de deep sleep %d horas y %d minutos", tiempo.horas, tiempo.minutos);
    esp_timer_start_once(deep_sleep_timer, us_hasta_dormir);
}

void time_sync_notification_cb(struct timeval *tv)
{
    ESP_LOGI(TAG, "Notification of a time synchronization event");
}

static void deep_sleep_timer_callback(void * args){
    int32_t horas = 0;
    //Calculo cuanto tiempo duermo
    if (HOUR_TO_SLEEP < HOUR_TO_WAKEUP){
        //En el mismo día
        horas = HOUR_TO_WAKEUP - HOUR_TO_SLEEP;
    } else {
        horas = 24 - HOUR_TO_SLEEP + HOUR_TO_WAKEUP;
    }
    ESP_LOGI(TAG, "Voy a dormir %d horas", horas);
    long long int sleep_time = horas * 60 * 60 * 1000 * 1000;
    esp_sleep_enable_timer_wakeup(sleep_time);
    esp_deep_sleep_start();
}

void sincTimeAndSleep(void)
{
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    // Is time set? If not, tm_year will be (1970 - 1900).
    if (timeinfo.tm_year < (2020 - 1900)) {
        ESP_LOGI(TAG, "Time is not set yet. Connecting to WiFi and getting time over NTP.");
        obtain_time();
        // update 'now' variable with current time
        time(&now);
    }
#ifdef CONFIG_SNTP_TIME_SYNC_METHOD_SMOOTH
    else {
        // add 500 ms error to the current system time.
        // Only to demonstrate a work of adjusting method!
        {
            ESP_LOGI(TAG, "Add a error for test adjtime");
            struct timeval tv_now;
            gettimeofday(&tv_now, NULL);
            int64_t cpu_time = (int64_t)tv_now.tv_sec * 1000000L + (int64_t)tv_now.tv_usec;
            int64_t error_time = cpu_time + 500 * 1000L;
            struct timeval tv_error = { .tv_sec = error_time / 1000000L, .tv_usec = error_time % 1000000L };
            settimeofday(&tv_error, NULL);
        }

        ESP_LOGI(TAG, "Time was set, now just adjusting it. Use SMOOTH SYNC method.");
        obtain_time();
        // update 'now' variable with current time
        time(&now);
    }
#endif

    char strftime_buf[64];

    // Set timezone to Eastern Standard Time and print local time
    setenv("TZ", "Europe/Madrid", 1);
    tzset();
    localtime_r(&now, &timeinfo);
    strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
    ESP_LOGI(TAG, "The current date/time in Madrid is: %s", strftime_buf);

    if (sntp_get_sync_mode() == SNTP_SYNC_MODE_SMOOTH) {
        struct timeval outdelta;
        while (sntp_get_sync_status() == SNTP_SYNC_STATUS_IN_PROGRESS) {
            adjtime(NULL, &outdelta);
            ESP_LOGI(TAG, "Waiting for adjusting time ... outdelta = %li sec: %li ms: %li us",
                        (long)outdelta.tv_sec,
                        outdelta.tv_usec/1000,
                        outdelta.tv_usec%1000);
            vTaskDelay(2000 / portTICK_PERIOD_MS);
        }
    }

    //Calculo cuánto queda hasta la hora de dormir y pongo un timer
    struct tiempo tiempo = tiempoHastaDormir(timeinfo);
    ESP_LOGI(TAG, "Iniciamos el timer de deep sleep %d horas y %d minutos", tiempo.horas, tiempo.minutos);
    const esp_timer_create_args_t deep_sleep_timer_args = {
        .callback = &deep_sleep_timer_callback,
        .name = "periodic"
    }; 
    esp_timer_create(&deep_sleep_timer_args, &deep_sleep_timer);
    long long int us_hasta_dormir = 1000000*60*(tiempo.minutos + 60*tiempo.horas);
    esp_timer_start_once(deep_sleep_timer, us_hasta_dormir);
}

static void obtain_time(void)
{
    initialize_sntp();

    // wait for time to be set
    time_t now = 0;
    struct tm timeinfo = { 0 };
    int retry = 0;
    const int retry_count = 10;
    while (sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET && ++retry < retry_count) {
        ESP_LOGI(TAG, "Waiting for system time to be set... (%d/%d)", retry, retry_count);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
    time(&now);
    localtime_r(&now, &timeinfo);
}

static void initialize_sntp(void)
{
    ESP_LOGI(TAG, "Initializing SNTP");
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "pool.ntp.org");
    sntp_set_time_sync_notification_cb(time_sync_notification_cb);
#ifdef CONFIG_SNTP_TIME_SYNC_METHOD_SMOOTH
    sntp_set_sync_mode(SNTP_SYNC_MODE_SMOOTH);
#endif
    sntp_init();
}

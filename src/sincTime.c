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

#define NTP_SYNC_PERIOD 60*60  // period in seconds for ntp timer

static int32_t HOUR_TO_SLEEP = CONFIG_HOUR_TO_SLEEP;
static int32_t HOUR_TO_WAKEUP = CONFIG_HOUR_TO_WAKEUP;

extern esp_err_t power_pin_down(void);

static const char *TAG = "sntp";
esp_timer_handle_t deep_sleep_timer;
esp_timer_handle_t ntp_timer;

static void obtain_time(void);
static void initialize_sntp(void);
static void deep_sleep_timer_callback(void * args);
static void ntp_timer_callback(void * args);

const esp_timer_create_args_t deep_sleep_timer_args = {
        .callback = &deep_sleep_timer_callback,
        .name = "periodic"
}; 

const esp_timer_create_args_t ntp_timer_args = {
        .callback = &ntp_timer_callback,
        .name = "ntp_timer",
		.arg = NULL,
}; 

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
    uint64_t us_hasta_dormir = 1000000*60*(tiempo.minutos + 60*tiempo.horas);
    ESP_LOGI(TAG, "Cambio: hora de dormir = %d, hora de despertar = %d", HOUR_TO_SLEEP, HOUR_TO_WAKEUP);
    ESP_LOGI(TAG, "Iniciamos el timer de deep sleep %d horas y %d minutos (%lld us)", tiempo.horas, tiempo.minutos,us_hasta_dormir);
    esp_timer_start_once(deep_sleep_timer, us_hasta_dormir);
}

void time_sync_notification_cb(struct timeval *tv)
{
    ESP_LOGI(TAG, "Notification of a time synchronization event");
}

static void deep_sleep_timer_callback(void * args){
    uint64_t horas = 0;
    //Calculo cuanto tiempo duermo
    if (HOUR_TO_SLEEP < HOUR_TO_WAKEUP){
        //En el mismo día
        horas = HOUR_TO_WAKEUP - HOUR_TO_SLEEP;
    } else {
        horas = 24 - HOUR_TO_SLEEP + HOUR_TO_WAKEUP;
    }
   
    uint64_t sleep_time = (uint64_t) (horas * 60 * 60 * 1000 * 1000);
     ESP_LOGI(TAG, "Voy a dormir %llu horas seguidas. %llu us", horas, sleep_time);
#ifdef CONFIG_SHUT_DOWN_POWER_PIN
    power_pin_down();
#endif

    esp_sleep_enable_timer_wakeup(sleep_time);
    esp_deep_sleep_start();
}

void wait_for_sntp_sync(time_t now)
{
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
}

static void ntp_timer_callback(void * args)
{
    time_t now;
	ESP_LOGI(TAG, "NTP Sync Timer callback.");
	obtain_time();
	time(&now);
	// FIXME: Not sure if this makes sense, the thread attending this casllback
	// will wait, but not the rest
	wait_for_sntp_sync(now);
}


void sincTimeAndSleep(void) {
#ifndef CONFIG_DEEP_SLEEP
        return;
#endif

    time_t now;
	ESP_LOGI(TAG, "Syncing time with remote ntp server.");
	obtain_time();
	time(&now);
    // Set timezone to Eastern Standard Time and print local time
    setenv("TZ", "Europe/Madrid", 1);
    tzset();

    char strftime_buf[64];
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);
    strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
    ESP_LOGI(TAG, "The current date/time in Madrid is: %s", strftime_buf);
	wait_for_sntp_sync(now);

    esp_timer_create(&ntp_timer_args, &ntp_timer);
    ESP_LOGI(TAG, "Starting time synchronization timer");
    esp_timer_start_periodic(ntp_timer, (uint64_t)NTP_SYNC_PERIOD * 1000000L);

    //Calculo cuánto queda hasta la hora de dormir y pongo un timer
    struct tiempo tiempo = tiempoHastaDormir(timeinfo);
    ESP_LOGI(TAG, "Iniciamos el timer de deep sleep %d horas y %d minutos", tiempo.horas, tiempo.minutos);

    esp_timer_create(&deep_sleep_timer_args, &deep_sleep_timer);
    uint64_t minutos_dormir = (long long int)(tiempo.minutos + 60*tiempo.horas);
    uint64_t us_hasta_dormir = (long long int) 1000000*60*minutos_dormir;
    ESP_LOGI(TAG, "Iniciamos el timer de deep sleep en %lld us",us_hasta_dormir);

    //ESP_LOGI(TAG,"Programado timer para deep sleep en %d us\n",us_hasta_dormir);
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

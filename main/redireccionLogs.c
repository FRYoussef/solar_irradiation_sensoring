#include <stdio.h>
#include "esp_log.h"
#include "mqtt_client.h"
#include "esp_timer.h"

#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"

#include "freertos/FreeRTOS.h"

#include <stdint.h>
#include <string.h>
#include "esp_vfs.h"
#include "esp_vfs_fat.h"

static const char *TAG = "LOGS";

// Handle of the wear levelling library instance
static wl_handle_t s_wl_handle = WL_INVALID_HANDLE;

// Mount path for the partition
const char *base_path = "/spiflash";

/*Redirección salida*/
static FILE * f;
static vprintf_like_t default_vprintf;

/*Timer*/
esp_timer_handle_t timer_sensor_logs;

int _log_vprintf(const char *fmt, va_list args) {
    int iresult;

    // Write to log.txt file
    if (f == NULL) {
        ESP_LOGE(TAG, "File handler is null\n");
        return -1;
    }
    iresult = vfprintf(f, fmt, args);
    if (iresult < 0) {
        ESP_LOGE(TAG, "Failed vfprintf() \n");       
    }

    return iresult;
}

static void timer_callback_logs(void * args){
    //Desredireccionamos los logs y mostramos algo del fichero
    //Cambio la salida a la estandar
    esp_log_set_vprintf(default_vprintf);

    //printf("Cierro el fichero\n");
    //Cierro el fichero abierto para escritura
    fclose(f);

    // Open file for reading
    //printf("Abrimos el fichero para lectura\n");
    f = fopen("/spiflash/logs.txt", "rb");
    if (f == NULL) {
        printf("Failed to open file for reading");
        return;
    }
    //printf("Leemos las 5 primeras lineas\n");
    for(int i = 0; i < 5; i++){
        char line[128];
        fgets(line, sizeof(line), f);
        // strip newline
        char *pos = strchr(line, '\n');
        if (pos) {
            *pos = '\0';
        }
        //printf("Read from file: '%s'\n", line);
    }

    fclose(f);
}

void redireccionaLogs(){
    /*Cambiamos a la partición spiflash*/
    ESP_LOGI(TAG, "Mounting FAT filesystem\n");
    const esp_vfs_fat_mount_config_t mount_config = {
            .max_files = 4,
            .format_if_mount_failed = true,
            .allocation_unit_size = CONFIG_WL_SECTOR_SIZE
    };
    ESP_ERROR_CHECK(esp_vfs_fat_spiflash_mount(base_path, "storage", &mount_config, &s_wl_handle));

    /*Redireccionamos la salida de los logs*/
    ESP_LOGI(TAG, "Opening file");
    f = fopen("/spiflash/logs.txt", "wb");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for writing");
        return;
    }

    //printf("Redireccionamos los logs\n");
    default_vprintf = esp_log_set_vprintf(&_log_vprintf);

    //Creamos un timer para dentro de 30 segundos, para salir de la redirección de logs
    ESP_LOGD(TAG, "Timer para salir de la redirección\n");
    const esp_timer_create_args_t timer_args = {
        .callback = &timer_callback_logs,
        .name = "periodic"
    }; 
    esp_timer_create(&timer_args, &timer_sensor_logs);
    esp_timer_start_once(timer_sensor_logs, 10*1000000);
}

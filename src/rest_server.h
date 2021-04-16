#include "sdkconfig.h"
#include "driver/gpio.h"
#include "esp_vfs_semihost.h"
#include "esp_vfs_fat.h"
#include "esp_spiffs.h"
#include "sdmmc_cmd.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_log.h"
#include "mdns.h"
#include "cJSON.h"
#include "cbor.h"
#include <string.h>
#include <fcntl.h>
#include "esp_http_server.h"
#include "esp_system.h"
#include "esp_vfs.h"
#include "lwip/apps/netbiosns.h"
#if CONFIG_EXAMPLE_WEB_DEPLOY_SD
#include "driver/sdmmc_host.h"
#endif

#define MDNS_INSTANCE "esp home web server"

esp_err_t start_rest_server(const char *base_path);

#define EXAMPLE_MDNS_HOST_NAME CONFIG_EXAMPLE_MDNS_HOST_NAME
#define EXAMPLE_WEB_MOUNT_POINT CONFIG_EXAMPLE_WEB_MOUNT_POINT
#define EXAMPLE_WEB_MOUNT_POINT CONFIG_EXAMPLE_WEB_MOUNT_POINT
#define EXAMPLE_HOST_PATH_TO_MOUNT CONFIG_EXAMPLE_HOST_PATH_TO_MOUNT

#define REST_CHECK(a, str, goto_tag, ...)                                              \
    do                                                                                 \
    {                                                                                  \
        if (!(a))                                                                      \
        {                                                                              \
            ESP_LOGE(TAG, "%s(%d): " str, __FUNCTION__, __LINE__, ##__VA_ARGS__); \
            goto goto_tag;                                                             \
        }                                                                              \
    } while (0)

#define FILE_PATH_MAX (ESP_VFS_PATH_MAX + 128)
#define SCRATCH_BUFSIZE (10240)

typedef struct rest_server_context {
    char base_path[ESP_VFS_PATH_MAX + 1];
    char scratch[SCRATCH_BUFSIZE];
} rest_server_context_t;

#define CHECK_FILE_EXTENSION(filename, ext) (strcasecmp(&filename[strlen(filename) - strlen(ext)], ext) == 0)
#include <string.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_http_client.h"
#include "esp_https_ota.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "protocol_examples_common.h"

#include "https_ota.h"
#include "mqtt_plugin.h"

#include "json_parser.h"

#if CONFIG_BOOTLOADER_APP_ANTI_ROLLBACK
#include "esp_efuse.h"
#endif

#if CONFIG_EXAMPLE_CONNECT_WIFI
#include "esp_wifi.h"
#endif

static const char *TAG = "advanced_https_ota_example";
/* Event handler for catching system events */

void build_ota_status_report(char *state, char *buffer, int buffer_size)
{
    // {"current_fw_title": "myFirmware", "current_fw_version": "1.2.3", "fw_state": "UPDATED"}
}

void mqtt_listener(char *topic, char *msg)
{

    ESP_LOGI(TAG, "topic %s", topic);
    ESP_LOGI(TAG, "msg %s", msg);

    jparse_ctx_t jctx;
    int err = json_parse_start(&jctx, msg, strlen(msg));
    if (err != OS_SUCCESS)
    {
        ESP_LOGE(TAG, "ERROR ON JSON PARSE: %d", err);
    }

    if (strcmp(topic, "v1/devices/me/attributes") == 0)
    {
        char fw_url[70];

        int err = json_obj_get_string(&jctx, "fw_url", fw_url, sizeof(fw_url));

        if (err == OS_SUCCESS)
        {
            ESP_LOGI(TAG, "installing new firmware from: %s", fw_url);
            advanced_ota_example_task(fw_url);

            mqtt_send_ota_status_report("DOWNLOADING");
        }
        else
        {
            ESP_LOGE(TAG, "ERROR ON JSON KEY EXTRACTION: %d", err);
        }
    }
}

void app_main(void)
{

    // switch (esp_reset_reason()) // https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/misc_system_api.html#_CPPv418esp_reset_reason_t
    // {
    // case /* constant-expression */:
    //     /* code */
    //     break;

    // default:
    //     break;
    // }

    // Initialize NVS.
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        // 1.OTA app partition table has a smaller NVS partition size than the non-OTA
        // partition table. This size mismatch may cause NVS initialization to fail.
        // 2.NVS partition contains data in new format and cannot be recognized by this version of code.
        // If this happens, we erase NVS partition and initialize NVS again.
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_ERROR_CHECK(esp_event_handler_register(ESP_HTTPS_OTA_EVENT, ESP_EVENT_ANY_ID, &ota_event_handler, NULL));
    ESP_ERROR_CHECK(example_connect());

    mqtt_init(mqtt_listener);

    const esp_partition_t *running = esp_ota_get_running_partition();
    esp_ota_img_states_t ota_state;
    if (esp_ota_get_state_partition(running, &ota_state) == ESP_OK)
    {
        if (ota_state == ESP_OTA_IMG_PENDING_VERIFY)
        {
            // TODO selfcheck
            if (true)
            {
                mqtt_send_ota_status_report("UPDATED");
                esp_ota_mark_app_valid_cancel_rollback();
            }
            else
            {
                mqtt_send_ota_fail("new image caused a rollback");
                esp_ota_mark_app_invalid_rollback_and_reboot();
            }
        }
    }

    esp_wifi_set_ps(WIFI_PS_NONE);

    mqtt_subscribe("v1/devices/me/attributes"); // check if it worked or needs retriying
    mqtt_send("v1/devices/me/telemetry", "{\"online\":true}");
}

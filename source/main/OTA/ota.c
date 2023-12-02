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
#include "bsp/esp-bsp.h"

#include "ota.h"
#include "../MQTT/mqtt.h"
#include "../Screen/screen.h"
#include "../common.h"
#include "../SYS_MODE/sys_mode.h"
#include "../common.h"

#if CONFIG_BOOTLOADER_APP_ANTI_ROLLBACK
#include "esp_efuse.h"
#endif

#if CONFIG_EXAMPLE_CONNECT_WIFI
#include "esp_wifi.h"
#endif

static const char *TAG = "ota";
extern const uint8_t server_cert_pem_start[] asm("_binary_ca_cert_pem_start");
extern const uint8_t server_cert_pem_end[] asm("_binary_ca_cert_pem_end");


void command_ota_state(enum OTAState OTA_state, struct OTAConf *conf)
{
    {
        struct MQTTMsg *msg = malloc(sizeof(struct MQTTMsg));
        msg->command = OTA_state_update;
        msg->data.ota_state_update.ota_state = OTA_state;

        int res = xQueueSend(conf->to_mqtt_queue, &msg, 0);
        if (res == pdFAIL)
        {
            free(msg);
        }
    }
    {
        struct ScreenMsg *msg = malloc(sizeof(struct ScreenMsg));
        msg->command = DisplayProcessing;
        char *ota_state_to_text[] = {
            "ota state is DOWNLOADING",
            "ota state is DOWNLOADED",
            "ota state is VERIFIED",
            "ota state is UPDATING",
            "ota state is UPDATED",
        };
        strcpy(msg->data.text, ota_state_to_text[OTA_state]);

        int res = xQueueSend(conf->to_screen_queue, &msg, 0);
        if (res == pdFAIL)
        {
            free(msg);
        }
    }
}

void command_ota_fail(char *error, struct OTAConf *conf)
{
    {
        struct MQTTMsg *msg = malloc(sizeof(struct MQTTMsg));
        msg->command = OTA_failure;
        msg->data.ota_failure.failure_msg = error;

        int res = xQueueSend(conf->to_mqtt_queue, &msg, 0);
        if (res == pdFAIL)
        {
            free(msg);
        }
    }
    {
        struct ScreenMsg *msg = malloc(sizeof(struct ScreenMsg));
        msg->command = DisplayError;
        strcpy(msg->data.text, error);

        int res = xQueueSend(conf->to_screen_queue, &msg, 0);
        if (res == pdFAIL)
        {
            free(msg);
        }
    }
}

static esp_err_t validate_image_header(esp_app_desc_t *new_app_info)
{
    if (new_app_info == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    const esp_partition_t *running = esp_ota_get_running_partition();
    esp_app_desc_t running_app_info;
    if (esp_ota_get_partition_description(running, &running_app_info) == ESP_OK)
    {
        ESP_LOGI(TAG, "Running firmware version: %s", running_app_info.version);
    }

#ifndef CONFIG_EXAMPLE_SKIP_VERSION_CHECK
    ESP_LOGI(TAG, "Updating firmware version to %s", new_app_info->version);

    if (memcmp(new_app_info->version, running_app_info.version, sizeof(new_app_info->version)) == 0)
    {
        ESP_LOGW(TAG, "Current running version is the same as a new. We will not continue the update.");
        return ESP_FAIL;
    }
#endif

#ifdef CONFIG_BOOTLOADER_APP_ANTI_ROLLBACK
    /**
     * Secure version check from firmware image header prevents subsequent download and flash write of
     * entire firmware image. However this is optional because it is also taken care in API
     * esp_https_ota_finish at the end of OTA update procedure.
     */
    const uint32_t hw_sec_version = esp_efuse_read_secure_version();
    if (new_app_info->secure_version < hw_sec_version)
    {
        ESP_LOGW(TAG, "New firmware security version is less than eFuse programmed, %" PRIu32 " < %" PRIu32, new_app_info->secure_version, hw_sec_version);
        return ESP_FAIL;
    }
#endif

    return ESP_OK;
}

static esp_err_t _http_client_init_cb(esp_http_client_handle_t http_client)
{
    esp_err_t err = ESP_OK;
    /* Uncomment to add custom headers to HTTP request */
    // err = esp_http_client_set_header(http_client, "Custom-Header", "Value");
    return err;
}

void ota_routine(char *url, struct OTAConf *conf)
{
    // esp_camera_deinit();
    // bsp_i2c_deinit();
    command_ota_state(DOWNLOADING, conf);

    esp_err_t ota_finish_err = ESP_OK;
    esp_http_client_config_t config = {
        .url = url,
        .cert_pem = (char *)server_cert_pem_start,
        .timeout_ms = CONFIG_EXAMPLE_OTA_RECV_TIMEOUT,
        .keep_alive_enable = true,
    };

    // config.skip_cert_common_name_check = true;

    esp_https_ota_config_t ota_config = {
        .http_config = &config,
        .http_client_init_cb = _http_client_init_cb, // Register a callback to be invoked after esp_http_client is initialized
        // .partial_http_download = false,
        // .max_http_request_size = 1024,
    };

    esp_https_ota_handle_t https_ota_handle = NULL;
    esp_err_t err = esp_https_ota_begin(&ota_config, &https_ota_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "ESP HTTPS OTA Begin failed");
        return;
    }

    esp_app_desc_t app_desc;
    err = esp_https_ota_get_img_desc(https_ota_handle, &app_desc);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "esp_https_ota_read_img_desc failed");
        goto ota_end;
    }
    err = validate_image_header(&app_desc);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "image header verification failed");
        goto ota_end;
    }
    ESP_LOGI(TAG, "image header is OK");

    while (1)
    {

        vTaskDelay(1);

        err = esp_https_ota_perform(https_ota_handle);

        if (err != ESP_ERR_HTTPS_OTA_IN_PROGRESS)
        {
            break;
        }

        // esp_https_ota_perform returns after every read operation which gives user the ability to
        // monitor the status of OTA upgrade by calling esp_https_ota_get_image_len_read, which gives length of image
        // data read so far.
        float progress = (float)esp_https_ota_get_image_len_read(https_ota_handle) / (float)esp_https_ota_get_image_size(https_ota_handle);

        // heap_caps_print_heap_info(0x00000008);
        // ESP_LOGI("xTaskCreateCap", "single largest posible allocation: %d", heap_caps_get_largest_free_block(0x00000008));

        ESP_LOGI(TAG, "Image bytes read: %f", progress);

        {
            struct ScreenMsg *msg = malloc(sizeof(struct ScreenMsg));
            msg->command = DisplayProgress;
            strcpy(msg->data.progress.text, "descargando:");
            msg->data.progress.progress = progress;

            int res = xQueueSend(conf->to_screen_queue, &msg, 0);
            if (res != pdTRUE)
            {
                free(msg);
            }
        }
    }
    ESP_LOGI(TAG, "out of download loop");

    if (esp_https_ota_is_complete_data_received(https_ota_handle) != true)
    {
        command_ota_fail("Complete data was not revieved", conf);

        // the OTA image was not completely received and user can customise the response to this situation.
        ESP_LOGE(TAG, "Complete data was not received.");
    }
    else
    {
        command_ota_state(DOWNLOADED, conf);

        ota_finish_err = esp_https_ota_finish(https_ota_handle);
        if ((err == ESP_OK) && (ota_finish_err == ESP_OK))
        {
            command_ota_state(VERIFIED, conf);

            ESP_LOGI(TAG, "ESP_HTTPS_OTA upgrade successful. Rebooting ...");
            vTaskDelay(1000 / portTICK_PERIOD_MS);

            command_ota_state(UPDATING, conf);

            vTaskDelay(1000 / portTICK_PERIOD_MS);

            esp_restart();
        }
        else
        {
            command_ota_fail("downloaded data couldnt be verified", conf);

            if (ota_finish_err == ESP_ERR_OTA_VALIDATE_FAILED)
            {
                ESP_LOGE(TAG, "Image validation failed, image is corrupted");
            }
            ESP_LOGE(TAG, "ESP_HTTPS_OTA upgrade failed 0x%x", ota_finish_err);
            return;
        }
    }

ota_end:
    esp_https_ota_abort(https_ota_handle);
    // esp_camera_init(conf->cam_conf);
    // bsp_i2c_init();

    ESP_LOGE(TAG, "ESP_HTTPS_OTA upgrade failed");
    return;
}

void ota_event_handler(void *arg, esp_event_base_t event_base,
                       int32_t event_id, void *event_data)
{
    if (event_base == ESP_HTTPS_OTA_EVENT)
    {
        switch (event_id)
        {
        case ESP_HTTPS_OTA_START:
            ESP_LOGI(TAG, "OTA started");
            break;
        case ESP_HTTPS_OTA_CONNECTED:
            ESP_LOGI(TAG, "Connected to server");
            break;
        case ESP_HTTPS_OTA_GET_IMG_DESC:
            ESP_LOGI(TAG, "Reading Image Description");
            break;
        case ESP_HTTPS_OTA_VERIFY_CHIP_ID:
            ESP_LOGI(TAG, "Verifying chip id of new image: %d", *(esp_chip_id_t *)event_data);
            break;
        case ESP_HTTPS_OTA_DECRYPT_CB:
            ESP_LOGI(TAG, "Callback to decrypt function");
            break;
        case ESP_HTTPS_OTA_WRITE_FLASH:
            ESP_LOGD(TAG, "Writing to flash: %d written", *(int *)event_data);
            break;
        case ESP_HTTPS_OTA_UPDATE_BOOT_PARTITION:
            ESP_LOGI(TAG, "Boot partition updated. Next Partition: %d", *(esp_partition_subtype_t *)event_data);
            break;
        case ESP_HTTPS_OTA_FINISH:
            ESP_LOGI(TAG, "OTA finish");
            break;
        case ESP_HTTPS_OTA_ABORT:
            ESP_LOGI(TAG, "OTA abort");
            break;
        }
    }
}

void ota_task(void *arg)
{
    struct OTAConf *conf = arg;
    while (1)
    {
        vTaskDelay(TASK_DELAY);
        // ESP_LOGI(TAG, "tick");

        struct OTAMsg *msg;

        int res = xQueueReceive(conf->to_ota_queue, &msg, 0);
        if (res != pdPASS)
        {
            continue;
        }
        set_mode(self_managed);
        ota_routine(msg->url, conf);
        free(msg);
    }
}

void ota_start(struct OTAConf *ota_conf)
{
    ESP_ERROR_CHECK(esp_event_handler_register(ESP_HTTPS_OTA_EVENT, ESP_EVENT_ANY_ID, &ota_event_handler, NULL));

    TaskHandle_t handler = jTaskCreate(&ota_task, "OTA task", 9000, ota_conf, 1, MALLOC_CAP_8BIT); // los ota deben ocurrir en un stack interno
    if (handler == NULL)
    {
        ESP_LOGE(TAG, "Problem on task start");
        heap_caps_print_heap_info(MALLOC_CAP_8BIT);
    }
}
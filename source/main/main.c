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

#include "Camera/camera.h"
#include "MQTT/mqtt.h"
#include "OTA/ota.h"
#include "QR/qr.h"
#include "Screen/screen.h"
#include "Starter/starter.h"

#include "json_parser.h"

#if CONFIG_BOOTLOADER_APP_ANTI_ROLLBACK
#include "esp_efuse.h"
#endif

#include "esp_wifi.h"

static const char *TAG = "tfgsegumientodocente";

void build_ota_status_report(char *state, char *buffer, int buffer_size)
{
    // {"current_fw_title": "myFirmware", "current_fw_version": "1.2.3", "fw_state": "UPDATED"}
}



void app_main(void)
{

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

    // build queues
    QueueHandle_t cam_to_qr_queue = xQueueCreate(1, sizeof(camera_fb_t *));
    QueueHandle_t qr_to_starter_queue = xQueueCreate(1, sizeof(StarterMsg *));
    QueueHandle_t starter_to_screen_queue = xQueueCreate(1, sizeof(ScreenMsg *));
    QueueHandle_t qr_to_mqtt_queue = xQueueCreate(1, sizeof(MQTTMsg *));
    QueueHandle_t mqtt_to_screen_queue = xQueueCreate(1, sizeof(ScreenMsg *));
    QueueHandle_t mqtt_to_ota_queue = xQueueCreate(1, sizeof(OTAMsg *));
    QueueHandle_t ota_to_mqtt_queue = xQueueCreate(1, sizeof(MQTTMsg *));
    QueueHandle_t ota_to_screen_queue = xQueueCreate(1, sizeof(ScreenMsg *));

    assert(cam_to_qr_queue);

    // Initialize the camera
    {
        camera_config_t camera_config = BSP_CAMERA_DEFAULT_CONFIG;
        camera_config.frame_size = CAM_FRAME_SIZE;

        CameraConf cam_conf = {
            .cam_to_qr_queue = cam_to_qr_queue,
            .camera_config = camera_config,
        }

        camera_start(cam_conf);
    }

    // Initialize QR
    {
        QRConf qr_conf = {
            .cam_to_qr_queue = cam_to_qr_queue,
            .qr_to_starter_queue = qr_to_starter_queue,
            .qr_to_mqtt_queue = qr_to_mqtt_queue,
        };

        qr_start(qr_conf)
    }

    // Initialize MQTT
    {
        MQTTConf mqtt_conf = {
            .qr_to_mqtt_queue = qr_to_mqtt_queue,
            .mqtt_to_ota_queue = mqtt_to_ota_queue,
            .ota_to_mqtt_queue = ota_to_mqtt_queue,
            .mqtt_to_screen_queue = mqtt_to_screen_queue,
            .starter_to_mqtt_queue = starter_to_mqtt_queue,
        };
        mqtt_start(mqtt_conf)
    }

    // Initialize OTA
    {
        OTAConf ota_conf{
            .ota_to_mqtt_queue = ota_to_mqtt_queue,
            .mqtt_to_ota_queue = mqtt_to_ota_queue,
            .ota_to_screen_queue = ota_to_screen_queue,
        };
        ota_start(ota_conf);
    }

    // Initialize Screen
    {
        ScreenConf screen_conf = {
            .starter_to_screen_queue = starter_to_screen_queue,
            .mqtt_to_screen_queue = mqtt_to_screen_queue,
            .ota_to_sceen_queue = ota_to_sceen_queue,
        };
        screen_start(screen_conf);
    }

    // Initialize Starter
    {

        StarterConf starter_conf = {
            .starter_to_screen_queue = starter_to_screen_queue,
            .qr_to_starter_queue = qr_to_starter_queue,
            .starter_to_mqtt_queue = starter_to_mqtt_queue,
        };
        start_starter(starter_conf);
    }

    // mqtt_subscribe("v1/devices/me/attributes"); // check if it worked or needs retriying
    // mqtt_send("v1/devices/me/telemetry", "{\"online\":true}");
}

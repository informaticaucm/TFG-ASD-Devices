#include <stdio.h>
#include <sys/param.h>
#include "esp_err.h"
#include "esp_log.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "esp_timer.h"
#include "esp_heap_caps.h"
#include "driver/spi_master.h"
#include "driver/sdmmc_host.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "bsp/esp-bsp.h"
#include "sdkconfig.h"
#include "quirc.h"
#include "src/misc/lv_color.h"

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "esp_camera.h"
#include "esp_ota_ops.h"
#include "json_parser.h"
#include "esp_wifi.h"

#include "Camera/camera.h"
#include "MQTT/mqtt.h"
#include "OTA/ota.h"
#include "QR/qr.h"
#include "Screen/screen.h"
#include "Starter/starter.h"
#include "Buttons/buttons.h"
#include "TOTP/totp.h"

#include "nvs_plugin.h"

#include "common.h"

static const char *TAG = "tfgsegumientodocente";

void app_main(void)
{

    // heap_caps_print_heap_info(0x00001800);
    // ESP_LOGE(TAG, "single largest posible allocation at startup: %d", heap_caps_get_largest_free_block(0x00001800));

    // heap_caps_print_heap_info(MALLOC_CAP_SPIRAM);
    // ESP_LOGE(TAG, "single largest posible allocation at startup at psram: %d", heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM));
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    bsp_i2c_init();
    bsp_display_cfg_t cfg = {
        .lvgl_port_cfg = {
            .task_priority = 0,
            .task_stack = 20000,
            .task_affinity = -1,
            .timer_period_ms = TASK_DELAY,
            .task_max_sleep_ms = TASK_DELAY * 2,
        }};
    bsp_display_start_with_config(&cfg);
    bsp_leds_init();

    bsp_led_set(BSP_LED_GREEN, false);

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

    const esp_partition_t *running = esp_ota_get_running_partition();
    esp_ota_img_states_t ota_state;
    int send_updated_mqtt_on_start = false;

    if (esp_ota_get_state_partition(running, &ota_state) == ESP_OK)
    {

        const char *otaStateStr = ota_state == ESP_OTA_IMG_NEW              ? "ESP_OTA_IMG_NEW"
                                  : ota_state == ESP_OTA_IMG_PENDING_VERIFY ? "ESP_OTA_IMG_PENDING_VERIFY"
                                  : ota_state == ESP_OTA_IMG_VALID          ? "ESP_OTA_IMG_VALID"
                                  : ota_state == ESP_OTA_IMG_INVALID        ? "ESP_OTA_IMG_INVALID"
                                  : ota_state == ESP_OTA_IMG_ABORTED        ? "ESP_OTA_IMG_ABORTED"
                                                                            : "ESP_OTA_IMG_UNDEFINED";
        ESP_LOGE(TAG, "ota_state: %s", otaStateStr);

        if (ota_state == ESP_OTA_IMG_PENDING_VERIFY)
        {
            // TODO selfcheck
            if (true)
            {
                send_updated_mqtt_on_start = true;
                esp_ota_mark_app_valid_cancel_rollback();
            }
            else
            {
                // TODO mqtt send ota fail
                esp_ota_mark_app_invalid_rollback_and_reboot();
            }
        }
    }

    // build queues
    QueueHandle_t to_qr_queue = xQueueCreate(1, sizeof(camera_fb_t *));
    QueueHandle_t to_starter_queue = xQueueCreate(10, sizeof(struct StarterMsg *));
    QueueHandle_t to_screen_queue = xQueueCreate(10, sizeof(struct ScreenMsg *));
    QueueHandle_t to_mqtt_queue = xQueueCreate(10, sizeof(struct MQTTMsg *));
    QueueHandle_t to_ota_queue = xQueueCreate(10, sizeof(struct OTAMsg *));

    assert(to_qr_queue);
    assert(to_starter_queue);
    assert(to_screen_queue);
    assert(to_mqtt_queue);
    assert(to_ota_queue);

    // Initialize QR

    struct quirc *qr = quirc_new();

    quirc_resize(qr, IMG_WIDTH, IMG_HEIGHT);

    struct QRConf *qr_conf = jalloc(sizeof(struct QRConf));
    qr_conf->to_qr_queue = to_qr_queue;
    qr_conf->to_starter_queue = to_starter_queue;
    qr_conf->to_mqtt_queue = to_mqtt_queue;

    qr_conf->qr = qr;

    qr_start(qr_conf);
    ESP_LOGI(TAG, "qr started");

    // Initialize MQTT

    struct MQTTConf *mqtt_conf = jalloc(sizeof(struct MQTTConf));
    mqtt_conf->to_mqtt_queue = to_mqtt_queue;
    mqtt_conf->to_ota_queue = to_ota_queue;
    mqtt_conf->to_screen_queue = to_screen_queue;
    mqtt_conf->to_starter_queue = to_starter_queue;

    mqtt_conf->send_updated_mqtt_on_start = send_updated_mqtt_on_start;

    mqtt_start(mqtt_conf);
    ESP_LOGI(TAG, "mqtt started");

    // Initialize the camera

    camera_config_t *camera_config = jalloc(sizeof(camera_config_t));
    {
        camera_config_t on_stack = BSP_CAMERA_DEFAULT_CONFIG;
        memcpy(camera_config, &on_stack, sizeof(camera_config_t));
    }
    camera_config->frame_size = CAM_FRAME_SIZE;

    struct CameraConf *cam_conf = jalloc(sizeof(struct CameraConf));
    cam_conf->to_qr_queue = to_qr_queue;
    cam_conf->to_screen_queue = to_screen_queue;
    cam_conf->camera_config = camera_config;
    camera_start(cam_conf);
    ESP_LOGI(TAG, "cam started");

    // Initialize OTA

    struct OTAConf *ota_conf = jalloc(sizeof(struct OTAConf));

    ota_conf->to_mqtt_queue = to_mqtt_queue;
    ota_conf->to_ota_queue = to_ota_queue;
    ota_conf->to_screen_queue = to_screen_queue;
    // ota_conf->cam_conf = camera_config;
    ota_start(ota_conf);
    ESP_LOGI(TAG, "ota started");

    // Initialize Screen

    struct ScreenConf *screen_conf = jalloc(sizeof(struct ScreenConf));
    screen_conf->to_screen_queue = to_screen_queue;

    screen_start(screen_conf);
    ESP_LOGI(TAG, "screen started");

    // Initialize Starter

    struct StarterConf *starter_conf = jalloc(sizeof(struct StarterConf));
    starter_conf->to_screen_queue = to_screen_queue;
    starter_conf->to_starter_queue = to_starter_queue;
    starter_conf->to_mqtt_queue = to_mqtt_queue;

    start_starter(starter_conf);
    ESP_LOGI(TAG, "starter started");

    // Initialize Buttons

    struct ButtonsConf *buttons_conf = jalloc(sizeof(struct ButtonsConf));

    buttons_start(buttons_conf);
    ESP_LOGI(TAG, "starter started");

    // Initialize TOTP

    struct TOTPConf *totp_conf = jalloc(sizeof(struct TOTPConf));
    totp_conf->to_screen_queue = to_screen_queue;

    start_totp(totp_conf);
    ESP_LOGI(TAG, "TOTP started");
}

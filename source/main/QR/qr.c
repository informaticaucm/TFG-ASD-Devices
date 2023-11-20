#include "qr.h"
#include "../Starter/starter.h"
#include "../MQTT/mqtt.h"
#include "../Camera/camera.h"
#include "../Screen/screen.h"

#include <stdio.h>
#include <sys/param.h>
#include "esp_err.h"
#include "esp_log.h"
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
#include "quirc_internal.h"
#include "esp_camera.h"
#include "src/misc/lv_color.h"
#include "json_parser.h"

#include "../common.h"

#define TAG "qr"

// Helper functions to convert an RGB565 image to grayscale
typedef union
{
    uint16_t val;
    struct
    {
        uint16_t b : 5;
        uint16_t g : 6;
        uint16_t r : 5;
    };
} rgb565_t;

static uint8_t rgb565_to_grayscale(const uint8_t *img)
{
    uint16_t *img_16 = (uint16_t *)img;
    rgb565_t rgb = {.val = __builtin_bswap16(*img_16)};
    uint16_t val = (rgb.r * 8 + rgb.g * 4 + rgb.b * 8) / 3;
    return (uint8_t)MIN(255, val);
}

static void rgb565_to_grayscale_buf(const uint8_t *src, uint8_t *dst, int qr_width, int qr_height)
{
    for (size_t y = 0; y < qr_height; y++)
    {
        for (size_t x = 0; x < qr_width; x++)
        {
            dst[y * qr_width + x] = rgb565_to_grayscale(&src[(y * qr_width + x) * 2]);
        }
    }
}

static void qr_task(void *arg)
{
    struct QRConf *conf = arg;

    struct quirc *qr = conf->qr;

    int frame = 0;
    ESP_LOGI(TAG, "Processing task ready");
    while (1)
    {
        vTaskDelay(TASK_DELAY);

        // ESP_LOGI(TAG, "tick");

        camera_fb_t *pic;
        uint8_t *qr_buf = quirc_begin(qr, NULL, NULL);

        // Get the next frame from the queue
        int res = xQueueReceive(conf->to_qr_queue, &pic, 0);
        if (res != pdPASS)
        {
            continue;
        }

        // Convert the frame to grayscale. We could have asked the camera for a grayscale frame,
        // but then the image on the display would be grayscale too.
        rgb565_to_grayscale_buf(pic->buf, qr_buf, IMG_WIDTH, IMG_HEIGHT);

        // Return the frame buffer to the camera driver ASAP to avoid DMA errors
        esp_camera_fb_return(pic);

        // Process the frame. This step find the corners of the QR code (capstones)
        // ESP_LOGE("D", "%p : %p at %d", (void *)qr, (void *)qr->flood_fill_vars, (int)((void *)&(qr->flood_fill_vars) - (void *)qr));
        // ESP_LOGE("define log", "%d %d %d", QUIRC_MAX_REGIONS, QUIRC_MAX_CAPSTONES, QUIRC_MAX_GRIDS);

        quirc_end(qr);
        ++frame;
        int count = quirc_count(qr);
        quirc_decode_error_t err = QUIRC_ERROR_DATA_UNDERFLOW;

        // If a QR code was detected, try to decode it:
        for (int i = 0; i < count; i++)
        {
            struct quirc_code code = {};
            struct quirc_data qr_data = {};
            // Extract raw QR code binary data (values of black/white modules)
            quirc_extract(qr, i, &code);
            quirc_flip(&code);

            // Decode the raw data. This step also performs error correction.
            err = quirc_decode(&code, &qr_data);
            if (err != 0)
            {
                ESP_LOGE(TAG, "QR err: %d, %s", err, quirc_strerror(err));

                {
                    struct CameraMsg *msg = jalloc(sizeof(struct CameraMsg));

                    msg->command = StreamToScreen;
                    msg->data.stream.time = jeppoch + CAM_BYPASS_TIME;
                    msg->data.stream.refreshRate = 10;

                    int res = xQueueSend(conf->to_cam_queue, &msg, 0);
                    if (res != pdTRUE)
                    {
                        free(msg);
                    }
                }
            }
            else
            {
                // Indicate that we have successfully decoded something by blinking an LED
                bsp_led_set(BSP_LED_GREEN, true);
                {
                    struct CameraMsg *msg = jalloc(sizeof(struct CameraMsg));

                    msg->command = StreamToScreen;
                    msg->data.stream.time = 0;
                    msg->data.stream.refreshRate = 10;

                    int res = xQueueSend(conf->to_cam_queue, &msg, 0);
                    if (res != pdTRUE)
                    {
                        free(msg);
                    }
                }
                ESP_LOGI(TAG, "Processing task ready");

                ESP_LOGI(TAG, "the contents were: %s", qr_data.payload);

                // TODO decide qr meaning and send to Starter or MQTT modules

                if (strncmp("reconf", (char *)qr_data.payload, 6) == 0)
                {
                    // reconf{"device_name":"DevicePrueba","mqtt_broker_url":"mqtts://thingsboard.asd:8883","provisioning_device_key":"o7l9pkujk2xgnixqlimv","provisioning_device_secret":"of8htwr0xmh65wjpz7qe","wifi_psw":"1234567890","wifi_ssid":"tfgseguimientodocente"}
                    struct StarterMsg *msg = jalloc(sizeof(struct StarterMsg));

                    msg->command = QrInfo;

                    jparse_ctx_t jctx;
                    json_parse_start(&jctx, (char *)(qr_data.payload + 6), qr_data.payload_len - 6);

                    json_obj_get_string(&jctx, "device_name", msg->data.qr.device_name, 50);
                    json_obj_get_string(&jctx, "mqtt_broker_url", msg->data.qr.mqtt_broker_url, URL_SIZE);
                    json_obj_get_string(&jctx, "provisioning_device_key", msg->data.qr.provisioning_device_key, 21);
                    json_obj_get_string(&jctx, "provisioning_device_secret", msg->data.qr.provisioning_device_secret, 21);
                    json_obj_get_string(&jctx, "wifi_psw", msg->data.qr.wifi_psw, 30);
                    json_obj_get_string(&jctx, "wifi_ssid", msg->data.qr.wifi_ssid, 30);
                    
                    ESP_LOGI(TAG, "json field device_name %s", msg->data.qr.device_name);
                    ESP_LOGI(TAG, "json field mqtt_broker_url %s", msg->data.qr.mqtt_broker_url);
                    ESP_LOGI(TAG, "json field provisioning_device_key %s", msg->data.qr.provisioning_device_key);
                    ESP_LOGI(TAG, "json field provisioning_device_secret %s", msg->data.qr.provisioning_device_secret);
                    ESP_LOGI(TAG, "json field wifi_psw %s", msg->data.qr.wifi_psw);
                    ESP_LOGI(TAG, "json field wifi_ssid %s", msg->data.qr.wifi_ssid);

                    int res = xQueueSend(conf->to_starter_queue, &msg, 0);
                    if (res != pdTRUE)
                    {
                        free(msg);
                    }
                }
                else
                {
                    // TODO es una tui, procesala debidamente
                }

                bsp_led_set(BSP_LED_GREEN, false);
            }
        }
    }
}

void qr_start(struct QRConf *conf)
{
    TaskHandle_t handle = jTaskCreate(&qr_task, "QR task", 50000, conf, 1, MALLOC_CAP_SPIRAM);
    if (handle == NULL)
    {
        ESP_LOGE(TAG, "Problem on task start");
    }
}

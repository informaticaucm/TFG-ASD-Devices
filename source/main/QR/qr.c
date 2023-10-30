#include "qr.h"

#include <stdio.h>
#include <sys/param.h>
#include "esp_err.h"
#include "esp_log.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "esp_timer.h"
#include "esp_vfs_fat.h"
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
#include "qrcode_classifier.h"
#include "../common.h"

static void ota_task(void *arg)
{
    struct quirc *qr = quirc_new();
    assert(qr);

    int qr_width = IMG_WIDTH;
    int qr_height = IMG_HEIGHT;
    if (quirc_resize(qr, qr_height, qr_height) < 0)
    {
        ESP_LOGE(TAG, "Failed to allocate QR buffer");
        return;
    }

    QueueHandle_t input_queue = (QueueHandle_t)arg;
    int frame = 0;
    ESP_LOGI(TAG, "Processing task ready");
    while (true)
    {
        camera_fb_t *pic;
        uint8_t *qr_buf = quirc_begin(qr, NULL, NULL);

        // Get the next frame from the queue
        int res = xQueueReceive(input_queue, &pic, portMAX_DELAY);
        assert(res == pdPASS);

        int64_t t_start = esp_timer_get_time();
        // Convert the frame to grayscale. We could have asked the camera for a grayscale frame,
        // but then the image on the display would be grayscale too.
        rgb565_to_grayscale_buf(pic->buf, qr_buf, qr_width, qr_height);

        // Return the frame buffer to the camera driver ASAP to avoid DMA errors
        esp_camera_fb_return(pic);

        // Process the frame. This step find the corners of the QR code (capstones)
        quirc_end(qr);
        ++frame;
        int64_t t_end_find = esp_timer_get_time();
        int count = quirc_count(qr);
        quirc_decode_error_t err = QUIRC_ERROR_DATA_UNDERFLOW;
        int time_find_ms = (int)(t_end_find - t_start) / 1000;
        ESP_LOGI(TAG, "QR count: %d   Heap: %d  Stack free: %d  time: %d ms",
                 count, heap_caps_get_free_size(MALLOC_CAP_DEFAULT),
                 uxTaskGetStackHighWaterMark(NULL), time_find_ms);

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
            int64_t t_end = esp_timer_get_time();
            int time_decode_ms = (int)(t_end - t_end_find) / 1000;
            ESP_LOGI(TAG, "Decoded in %d ms", time_decode_ms);
            if (err != 0)
            {
                ESP_LOGE(TAG, "QR err: %d", err);
            }
            else
            {
                // Indicate that we have successfully decoded something by blinking an LED
                bsp_led_set(BSP_LED_GREEN, true);

                if (strstr((const char *)qr_data.payload, "COLOR:") != NULL)
                {
                    // If the QR code contains a color string, fill the display with the same color
                    int r = 0, g = 0, b = 0;
                    sscanf((const char *)qr_data.payload, "COLOR:%02x%02x%02x", &r, &g, &b);
                    ESP_LOGI(TAG, "QR code: COLOR(%d, %d, %d)", r, g, b);
                    display_set_color(r, g, b);
                }
                else
                {
                    ESP_LOGI(TAG, "QR code: %d bytes: '%s'", qr_data.payload_len, qr_data.payload);
                    // Otherwise, use the rules defined in qrclass.txt to find the image to display,
                    // based on the kind of data in the QR code.
                    const char *filename = classifier_get_pic_from_qrcode_data((const char *)qr_data.payload);
                    if (filename == NULL)
                    {
                        ESP_LOGI(TAG, "Classifier returned NULL");
                    }
                    else
                    {
                        ESP_LOGI(TAG, "Classified as '%s'", filename);
                        // The classifier should return the image file name to display.
                        display_set_icon(filename);
                    }
                }
                bsp_led_set(BSP_LED_GREEN, false);
            }
        }
    }
}

void qr_start(QRConf conf)
{
    xTaskCreate(&ota_task, "QR task", 35000, &conf, 1, NULL);
}
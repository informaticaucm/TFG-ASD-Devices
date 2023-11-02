#include "camera.h"
#include "../QR/qr.h"
#include "esp_log.h"
#include "../common.h"
#define TAG "camera"

void camera_task(void *arg)
{
    struct CameraConf *conf = arg;

    while (1)
    {
        vTaskDelay(TASK_DELAY);
        // ESP_LOGI(TAG, "tick");

        camera_fb_t *pic = esp_camera_fb_get();
        if (pic == NULL)
        {
            ESP_LOGE(TAG, "Get frame failed");
            continue;
        }
        // Don't update the display if the display image was just updated
        // (i.e. is still frozen)

        // Send the frame to the processing task.
        // Note the short delay — if the processing task is busy, simply drop the frame.
        int res = xQueueSend(conf->cam_to_qr_queue, &pic, 0);
        if (res == pdFAIL)
        {
            esp_camera_fb_return(pic);
        }
    }
}

void camera_start(struct CameraConf *conf)
{

    ESP_ERROR_CHECK(esp_camera_init(&conf->camera_config));
    sensor_t *s = esp_camera_sensor_get();
    s->set_vflip(s, 1);
    ESP_LOGI(TAG, "Camera Init done");
    int err = xTaskCreate(&camera_task, "Camera Task", 5000, conf, 1, NULL);
    if (err != pdPASS)
    {
        ESP_LOGE(TAG, "Problem on task start %s ", esp_err_to_name(err));
        heap_caps_print_heap_info(MALLOC_CAP_DEFAULT);
    }
}

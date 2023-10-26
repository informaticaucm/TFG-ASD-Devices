#include "camera.h"

struct CamTaskConf
{
    QueueHandle_t cam_to_qr_queue;
};

void camera_start(CameraConf conf)
{

    ESP_ERROR_CHECK(esp_camera_init(&conf.camera_config));
    sensor_t *s = esp_camera_sensor_get();
    s->set_vflip(s, 1);
    ESP_LOGI(TAG, "Camera Init done");

    CamTaskConf *arg = malloc(sizeof(CamTaskConf));
    arg->cam_to_qr_queue = conf.cam_to_qr_queue;

    xTaskCreatePinnedToCore(&camera_task, "Camera Task", 35000, arg, 1, NULL, 0);
}

void camera_task(void *arg)
{
    CamTaskConf *conf = (CamTaskConf *)arg;

    while (1)
    {
        camera_fb_t *pic = esp_camera_fb_get();
        if (pic == NULL)
        {
            ESP_LOGE(TAG, "Get frame failed");
            continue;
        }
        // Don't update the display if the display image was just updated
        // (i.e. is still frozen)
        if (esp_timer_get_time() < s_freeze_canvas_until)
        {
            esp_camera_fb_return(pic);
            continue;
        }

        // Send the frame to the processing task.
        // Note the short delay — if the processing task is busy, simply drop the frame.
        int res = xQueueSend(conf->cam_to_qr_queue, &pic, pdMS_TO_TICKS(10));
        if (res == pdFAIL)
        {
            esp_camera_fb_return(pic);
        }
    }
}
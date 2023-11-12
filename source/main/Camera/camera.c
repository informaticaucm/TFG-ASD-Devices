#include "camera.h"
#include "../QR/qr.h"
#include "../Screen/screen.h"
#include "esp_log.h"
#include "../common.h"
#include "esp_timer.h"
#include <string.h>

#define TAG "camera"
#define force_stream true

void camera_task(void *arg)
{
    struct CameraConf *conf = arg;

    int stream_refresh_rate = 0;
    int stream_end_time = 0;

    while (1)
    {
        if (stream_end_time > esp_timer_get_time() || force_stream)
        {
            vTaskDelay(stream_refresh_rate);
        }
        else
        {
            vTaskDelay(TASK_DELAY);
        }

        struct CameraMsg *msg;
        if (xQueueReceive(conf->to_cam_queue, &msg, 0) == pdPASS)
        {
            switch (msg->command)
            {
            case StreamToScreen:
                stream_refresh_rate = msg->data.stream.refreshRate;
                stream_end_time = msg->data.stream.time;
                break;
            }

            free(msg);
        }

        camera_fb_t *pic = esp_camera_fb_get();
        if (pic == NULL)
        {
            ESP_LOGE(TAG, "Get frame failed");
            continue;
        }

        if (stream_end_time > esp_timer_get_time() || force_stream)
        {

            struct ScreenMsg *msg = jalloc(sizeof(struct ScreenMsg));
            msg->command = DisplayImage;
            msg->data.image.buf = jalloc(pic->len);
            msg->data.image.height = pic->height;
            msg->data.image.width = pic->width;

            memcpy(msg->data.image.buf, pic->buf, pic->len);

            int res = xQueueSend(conf->to_screen_queue, &msg, 0);
            if (res != pdTRUE)
            {
                free(msg->data.image.buf);
                free(msg);
            }
        }

        int res = xQueueSend(conf->to_qr_queue, &pic, 0);
        if (res == pdFAIL)
        {
            esp_camera_fb_return(pic);
        }
    }
}

void camera_start(struct CameraConf *conf)
{

    ESP_ERROR_CHECK(esp_camera_init(conf->camera_config));
    sensor_t *s = esp_camera_sensor_get();
    s->set_vflip(s, 1);
    ESP_LOGI(TAG, "Camera Init done");
    TaskHandle_t handle = jTaskCreate(&camera_task, "Camera Task", 5000, conf, 1, MALLOC_CAP_SPIRAM);
    if (handle == NULL)
    {
        ESP_LOGE(TAG, "Problem on task start");
        heap_caps_print_heap_info(MALLOC_CAP_DEFAULT);
    }
}

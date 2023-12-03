#include "camera.h"
#include "../QR/qr.h"
#include "../Screen/screen.h"
#include "esp_log.h"
#include "../common.h"
#include "esp_timer.h"
#include <string.h>
#include "../SYS_MODE/sys_mode.h"

#define TAG "camera"

void camera_task(void *arg)
{
    struct CameraConf *conf = arg;

    while (1)
    {
        // ESP_LOGE(TAG, "stream_end_time: %d, jeppoch: %d", stream_end_time, (int)jeppoch);
        if (get_mode() == mirror)
        {
            vTaskDelay(MIRROR_RESHESH_RATE);
        }
        else
        {
            vTaskDelay(TASK_DELAY);
        }

        // struct CameraMsg *msg;
        // if (xQueueReceive(conf->to_cam_queue, &msg, 0) == pdPASS)
        // {
        //     switch (msg->command)
        //     {
        //     case StreamToScreen:
        //         stream_refresh_rate = msg->data.stream.refreshRate;
        //         stream_end_time = msg->data.stream.time;
        //         break;
        //     }

        //     free(msg);
        // }

        camera_fb_t *pic = esp_camera_fb_get();
        if (pic == NULL)
        {
            ESP_LOGE(TAG, "Get frame failed");
            continue;
        }

        if (get_mode() == mirror)
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
                ESP_LOGE(TAG, "mesage send error");
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

    // heap_caps_print_heap_info(0x00000404);

    ESP_ERROR_CHECK(esp_camera_init(conf->camera_config));
    sensor_t *s = esp_camera_sensor_get();
    s->set_vflip(s, 1);
    ESP_LOGI(TAG, "Camera Init done");
    TaskHandle_t handle = jTaskCreate(&camera_task, "Camera Task", 5000, conf, 1, MALLOC_CAP_SPIRAM);
    if (handle == NULL)
    {
        ESP_LOGE(TAG, "Problem on task start");
        heap_caps_print_heap_info(MALLOC_CAP_SPIRAM);
    }
}

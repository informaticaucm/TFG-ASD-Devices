#include "camera.h"
#include "../QR/qr.h"
#include "../Screen/screen.h"
#include "esp_log.h"
#include "../common.h"
#include "esp_timer.h"
#include <string.h>
#include "../SYS_MODE/sys_mode.h"

#define TAG "camera"

void meta_frame_write(struct meta_frame *frame, uint8_t *buf)
{
    if (frame->state == empty)
    {
        memcpy(frame->buf, buf, sizeof(frame->buf));
        frame->state = written;
    }
}

void meta_frame_free(struct meta_frame *frame)
{
    frame->state = empty;
}

#define MF_COUNT 3

struct meta_frame *metaframe_heap;

struct meta_frame *get_meta_frame()
{
    for (int i = 0; i < MF_COUNT; i++)
    {
        if (metaframe_heap[i].state == empty)
        {
            return &metaframe_heap[i];
        }
    }
    return NULL;
}

void camera_task(void *arg)
{
    struct CameraConf *conf = arg;

    while (1)
    {
        if (is_ota_running())
        {
            vTaskDelay(get_task_delay());
            continue;
        }
        // ESP_LOGE(TAG, "stream_end_time: %d, jeppoch: %d", stream_end_time, (int)jeppoch);
        if (get_mode() == mirror)
        {
            vTaskDelay(get_rt_task_delay());
        }
        else
        {
            vTaskDelay(get_task_delay());
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

        struct meta_frame *qr_mf = get_meta_frame();
        if (qr_mf != NULL)
        {
            meta_frame_write(qr_mf, pic->buf);
            int res = xQueueSend(conf->to_qr_queue, &qr_mf, 0);
            if (res == pdFAIL)
            {
                meta_frame_free(qr_mf);
            }
        }

        if (get_mode() == mirror)
        {

            struct meta_frame *mf = get_meta_frame();
            if (mf != NULL)
            {
                meta_frame_write(mf, pic->buf);

                struct ScreenMsg *msg = jalloc(sizeof(struct ScreenMsg));
                msg->command = DisplayImage;
                msg->data.mf = mf;

                int res = xQueueSend(conf->to_screen_queue, &msg, 0);
                if (res != pdTRUE)
                {
                    ESP_LOGE(TAG, "mesage send error");
                    free(msg);
                    meta_frame_free(mf);
                }
            }
            else
            {
                ESP_LOGE(TAG, "no meta frame available");
            }
        }

        esp_camera_fb_return(pic);
    }
}

void camera_start(struct CameraConf *conf)
{

    metaframe_heap = jalloc(MF_COUNT * sizeof(struct meta_frame));
    for (size_t i = 0; i < MF_COUNT; i++)
    {
        metaframe_heap[i].state = empty;
    }

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

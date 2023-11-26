#include "totp.h"
#include "../common.h"
#include "../SYS_MODE/sys_mode.h"
#include "../Screen/screen.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#define TAG "totp"

static void totp_task(void *arg)
{
    struct TOTPConf *conf = arg;

    while (1)
    {
        vTaskDelay(TASK_DELAY);

        if (get_mode() == qr_display)
        {
            {
                struct ScreenMsg *msg = jalloc(sizeof(struct ScreenMsg));

                msg->command = DrawQr;
                memcpy(msg->data.text, "holaa, esto va a ser un qr con una url y otras cosas chungas", MAX_QR_SIZE);

                int res = xQueueSend(conf->to_screen_queue, &msg, 0);
                if (res == pdFAIL)
                {
                    free(msg);
                }
            }
        }
    }
}

void start_totp(struct TOTPConf *conf)
{
    TaskHandle_t handle = jTaskCreate(&totp_task, "totp task", 25000, conf, 1, MALLOC_CAP_SPIRAM);
    if (handle == NULL)
    {
        ESP_LOGE(TAG, "Problem on task start ");
        heap_caps_print_heap_info(MALLOC_CAP_SPIRAM);
    }
}
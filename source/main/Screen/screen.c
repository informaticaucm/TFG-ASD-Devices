#include "screen.h"
#include "bsp/esp-bsp.h"
#include "../common.h"
#include "esp_log.h"

#define TAG "screen"

void screen_task(void *conf)
{
    while (1)
    {
        vTaskDelay(TASK_DELAY);
        //     if (bsp_display_lock(10))
        //     {
        //         bsp_display_unlock();
        //     }
    }
}

void screen_start(struct ScreenConf *conf)
{
    int err = xTaskCreate(&screen_task, "Screen task", 10000, conf, 1, NULL);
    if (err != pdPASS)
    {
        ESP_LOGE(TAG, "Problem on task start");
    }
}
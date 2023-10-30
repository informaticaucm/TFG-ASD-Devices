#include "screen.h"
#include "bsp/esp-bsp.h"

void screen_task(void *conf)
{
    // while (1)
    // {
    // vTaskDelay(1000 / portTICK_PERIOD_MS);
    //     if (bsp_display_lock(10))
    //     {
    //         bsp_display_unlock();
    //     }
    // }
}

void screen_start(struct ScreenConf *conf)
{
    xTaskCreate(&screen_task, "Screen task", 35000, conf, 1, NULL);
}
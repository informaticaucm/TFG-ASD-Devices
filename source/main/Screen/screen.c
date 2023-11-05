#include <stdio.h>
#include <sys/param.h>
#include "esp_err.h"
#include "esp_log.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
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
#include "src/misc/lv_color.h"

#include "screen.h"
#include "bsp/esp-bsp.h"
#include "../common.h"
#include "esp_log.h"

#define TAG "screen"

void screen_task(void *arg)
{
    struct ScreenConf *conf = arg;

    while (1)
    {
        vTaskDelay(TASK_DELAY);
        struct ScreenMsg *msg;

        if (xQueueReceive(conf->ota_to_screen_queue, &msg, 0) != pdPASS &&
            xQueueReceive(conf->mqtt_to_screen_queue, &msg, 0) != pdPASS &&
            xQueueReceive(conf->starter_to_screen_queue, &msg, 0) != pdPASS)
        {
            continue;
        }
        bsp_display_lock(0);

        lv_obj_clean(lv_scr_act());

        switch (msg->command)
        {
        case DisplayWarning:
        case DisplayInfo:
        case DisplayError:

        {
            lv_obj_t *lable = lv_label_create(lv_scr_act());
            lv_label_set_text(lable, msg->data.text);
            lv_obj_set_width(lable, 150);
            lv_obj_align(lable, LV_ALIGN_CENTER, 0, 0);
            break;
        }
        case DisplayProgress:

        {
            lv_obj_t *lable = lv_label_create(lv_scr_act());
            lv_label_set_text(lable, msg->data.progress.text);
            lv_obj_set_width(lable, 150);
            lv_obj_align(lable, LV_ALIGN_CENTER, 0, -30);

            lv_obj_t *bar = lv_bar_create(lv_scr_act());
            lv_obj_set_size(bar, 200, 20);
            lv_obj_align(bar, LV_ALIGN_CENTER, 0, 30);

            int bar_progress = (int)(msg->data.progress.progress * 100.);

            ESP_LOGE("log", "bar_progress: %d from %f", bar_progress, msg->data.progress.progress);
            lv_bar_set_value(bar, bar_progress, LV_ANIM_OFF);
            break;
        }
        case DisplayProcessing:
        {
            lv_obj_t *lable = lv_label_create(lv_scr_act());
            lv_label_set_text(lable, msg->data.text);
            lv_obj_set_width(lable, 150);
            lv_obj_align(lable, LV_ALIGN_CENTER, 0, -30);

            lv_obj_t *gif = lv_label_create(lv_scr_act());
            lv_label_set_text(gif, "aqui un gif de carga precioso");
            lv_obj_set_width(gif, 150);
            lv_obj_align(gif, LV_ALIGN_CENTER, 0, 30);
            break;
        }
        default:
        }
        bsp_display_unlock();

        free(msg);
    }
}

void screen_start(struct ScreenConf *conf)
{

    bsp_display_backlight_on();

    TaskHandle_t handle = jTaskCreate(&screen_task, "Screen task", 50000, conf, 1, MALLOC_CAP_SPIRAM);
    if (handle == NULL)
    {
        ESP_LOGE(TAG, "Problem on task start ");
        heap_caps_print_heap_info(MALLOC_CAP_DEFAULT);
    }
}
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

int last_bar_progress = 0;

void screen_task(void *arg)
{
    struct ScreenConf *conf = arg;

    static lv_style_t style_bar_bg;

    lv_style_init(&style_bar_bg);
    lv_style_set_border_color(&style_bar_bg, lv_palette_main(LV_PALETTE_BLUE));
    lv_style_set_border_width(&style_bar_bg, 2);
    lv_style_set_pad_all(&style_bar_bg, 6); /*To make the indicator smaller*/
    lv_style_set_radius(&style_bar_bg, 6);
    lv_style_set_anim_time(&style_bar_bg, 1000);

    static lv_style_t style_bar_indic;

    lv_style_init(&style_bar_indic);
    lv_style_set_bg_opa(&style_bar_indic, LV_OPA_COVER);
    lv_style_set_bg_color(&style_bar_indic, lv_palette_main(LV_PALETTE_ORANGE));
    lv_style_set_radius(&style_bar_indic, 3);

    static lv_style_t bg_style;
    lv_style_set_bg_color(&bg_style, lv_color_make(0x00, 0x00, 0x00));
    lv_obj_add_style(lv_scr_act(), &bg_style, LV_PART_MAIN);

    static lv_style_t label_style;
    lv_style_set_text_color(&label_style, lv_color_make(0xff, 0xff, 0xff));

    while (1)
    {
        vTaskDelay(RT_TASK_DELAY);
        struct ScreenMsg *msg;

        if (xQueueReceive(conf->ota_to_screen_queue, &msg, 0) != pdPASS &&
            xQueueReceive(conf->mqtt_to_screen_queue, &msg, 0) != pdPASS &&
            xQueueReceive(conf->qr_to_screen_demo_queue, &msg, 0) != pdPASS &&
            xQueueReceive(conf->starter_to_screen_queue, &msg, 0) != pdPASS)
        {
            continue;
        }
        bsp_display_lock(0);

        lv_obj_clean(lv_scr_act());

        switch (msg->command)
        {
        case DisplayWarning:
        {
            lv_obj_t *lable = lv_label_create(lv_scr_act());
            lv_label_set_text_fmt(lable, "Warning: %s", msg->data.text);
            lv_obj_set_width(lable, 150);
            lv_obj_align(lable, LV_ALIGN_CENTER, 0, 0);
            lv_obj_add_style(lable, &label_style, LV_PART_MAIN);
            break;
        }
        case DisplayInfo:
        {
            lv_obj_t *lable = lv_label_create(lv_scr_act());
            lv_label_set_text_fmt(lable, "Info: %s", msg->data.text);
            lv_obj_set_width(lable, 150);
            lv_obj_align(lable, LV_ALIGN_CENTER, 0, 0);
            lv_obj_add_style(lable, &label_style, LV_PART_MAIN);

            break;
        }
        case DisplayError:

        {
            lv_obj_t *lable = lv_label_create(lv_scr_act());
            lv_label_set_text_fmt(lable, "Error: %s", msg->data.text);
            lv_obj_set_width(lable, 150);
            lv_obj_align(lable, LV_ALIGN_CENTER, 0, 0);
            lv_obj_add_style(lable, &label_style, LV_PART_MAIN);

            break;
        }
        case DisplayProgress:
        {

            lv_obj_t *lable = lv_label_create(lv_scr_act());
            lv_label_set_text_fmt(lable, "%s \n(%f%%)", msg->data.progress.text, msg->data.progress.progress * 100.);
            lv_obj_set_width(lable, 150);
            lv_obj_align(lable, LV_ALIGN_CENTER, 0, -30);
            lv_obj_add_style(lable, &label_style, LV_PART_MAIN);

            lv_obj_t *bar = lv_bar_create(lv_scr_act());
            lv_obj_set_size(bar, 200, 20);
            lv_obj_align(bar, LV_ALIGN_CENTER, 0, 30);
            lv_obj_add_style(bar, &style_bar_bg, 0);
            lv_obj_add_style(bar, &style_bar_indic, LV_PART_INDICATOR);

            int bar_progress = (int)(msg->data.progress.progress * 100.);
            lv_bar_set_value(bar, last_bar_progress, LV_ANIM_OFF);
            last_bar_progress = bar_progress;
            lv_bar_set_value(bar, bar_progress, LV_ANIM_ON);
            break;
        }
        case DisplayProcessing:
        {
            lv_obj_t *lable = lv_label_create(lv_scr_act());
            lv_label_set_text(lable, msg->data.text);
            lv_obj_set_width(lable, 150);
            lv_obj_align(lable, LV_ALIGN_CENTER, 0, -30);
            lv_obj_add_style(lable, &label_style, LV_PART_MAIN);

            lv_obj_t *gif = lv_label_create(lv_scr_act());
            lv_label_set_text(gif, "aqui un gif de carga precioso");
            lv_obj_set_width(gif, 150);
            lv_obj_align(gif, LV_ALIGN_CENTER, 0, 30);
            lv_obj_add_style(gif, &label_style, LV_PART_MAIN);

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
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
#include "../icon/icon.h"
#include "../SYS_MODE/sys_mode.h"

#define TAG "screen"

int last_bar_progress = 0;

struct ScreenMsg *current_state;

void screen_task(void *arg)
{
    struct ScreenConf *conf = arg;

    current_state = jalloc(sizeof(struct ScreenMsg));
    current_state->command = Empty;
    uint8_t *canvas_buf = jalloc(1);

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
    lv_style_set_bg_color(&bg_style, lv_color_white());
    lv_obj_add_style(lv_scr_act(), &bg_style, LV_PART_MAIN);

    static lv_style_t label_style;
    lv_style_set_text_color(&label_style, lv_color_black());

    while (1)
    {
        struct ScreenMsg *msg;

        if (xQueueReceive(conf->to_screen_queue, &msg, get_rt_task_delay()) == pdPASS)
        {
            if (current_state->command == DisplayImage)
            {
                free(current_state->data.image.buf);
            }
            free(current_state);
            current_state = msg;
        }

        bsp_display_lock(0);

        lv_obj_clean(lv_scr_act());

        // lv_obj_t *time = lv_label_create(lv_scr_act());
        // lv_label_set_text_fmt(time, "time: %llds", esp_timer_get_time()/1000000);
        // lv_obj_set_width(time, 150);
        // lv_obj_align(time, LV_ALIGN_CENTER, 0, -90);
        // lv_obj_add_style(time, &label_style, LV_PART_MAIN);
        // ESP_LOGI(TAG, "screen task tick");

        switch (current_state->command)
        {
        case Empty:
        {

            lv_obj_t *lable = lv_label_create(lv_scr_act());
            lv_label_set_text(lable, "Empty");
            lv_obj_set_width(lable, 150);
            lv_obj_align(lable, LV_ALIGN_CENTER, 0, 0);
            lv_obj_add_style(lable, &label_style, LV_PART_MAIN);
            break;
        }
        case DisplayWarning:
        {
            lv_obj_t *icon = lv_img_create(lv_scr_act());
            lv_img_set_src(icon, &warning);
            lv_obj_align(icon, LV_ALIGN_CENTER, 0, 0);

            lv_obj_t *lable = lv_label_create(lv_scr_act());
            lv_label_set_text_fmt(lable, "Aviso: %s", current_state->data.text);
            lv_obj_set_width(lable, 150);
            lv_obj_align(lable, LV_ALIGN_CENTER, 0, 60);
            lv_obj_add_style(lable, &label_style, LV_PART_MAIN);
            break;
        }
        case DisplaySuccess:
        {

            lv_obj_t *icon = lv_img_create(lv_scr_act());
            lv_img_set_src(icon, &success);
            lv_obj_align(icon, LV_ALIGN_CENTER, 0, 0);

            lv_obj_t *lable = lv_label_create(lv_scr_act());
            lv_label_set_text_fmt(lable, "Exito: %s", current_state->data.text);
            lv_obj_set_width(lable, 150);
            lv_obj_align(lable, LV_ALIGN_CENTER, 0, 60);
            lv_obj_add_style(lable, &label_style, LV_PART_MAIN);

            break;
        }
        case DisplayError:
        {
            lv_obj_t *icon = lv_img_create(lv_scr_act());
            lv_img_set_src(icon, &failure);
            lv_obj_align(icon, LV_ALIGN_CENTER, 0, 0);

            lv_obj_t *lable = lv_label_create(lv_scr_act());
            lv_label_set_text_fmt(lable, "Error: %s", current_state->data.text);
            lv_obj_set_width(lable, 150);
            lv_obj_align(lable, LV_ALIGN_CENTER, 0, 60);
            lv_obj_add_style(lable, &label_style, LV_PART_MAIN);

            break;
        }
        case DisplayText:
        {

            lv_obj_t *lable = lv_label_create(lv_scr_act());
            lv_label_set_text_fmt(lable, "%s", current_state->data.text);
            lv_obj_set_width(lable, 150);
            lv_obj_align(lable, LV_ALIGN_CENTER, 0, 0);
            lv_obj_add_style(lable, &label_style, LV_PART_MAIN);

            break;
        }
        case DisplayProgress:
        {

            lv_obj_t *lable = lv_label_create(lv_scr_act());
            lv_label_set_text_fmt(lable, "%s \n(%f%%)", current_state->data.progress.text, current_state->data.progress.progress * 100.);
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
            lv_label_set_text(lable, current_state->data.text);
            lv_obj_set_width(lable, 150);
            lv_obj_align(lable, LV_ALIGN_CENTER, 0, -30);
            lv_obj_add_style(lable, &label_style, LV_PART_MAIN);

            lv_obj_t *bar = lv_bar_create(lv_scr_act());
            lv_obj_set_size(bar, 200, 20);
            lv_obj_align(bar, LV_ALIGN_CENTER, 0, 30);
            lv_obj_add_style(bar, &style_bar_bg, 0);
            lv_obj_add_style(bar, &style_bar_indic, LV_PART_INDICATOR);

            int bar_progress = esp_timer_get_time() / 10000 % 100;

            lv_bar_set_value(bar, bar_progress, LV_ANIM_ON);
            break;

            break;
        }
        case DrawQr:
        {

            lv_obj_t *qr = lv_qrcode_create(lv_scr_act(), 240, lv_color_black(), lv_color_white());
            /*Set data*/
            lv_qrcode_update(qr, current_state->data.text, strlen(current_state->data.text));
            lv_obj_center(qr);

            break;
        }
        case DisplayImage:
        {

            lv_obj_t *image_canvas = lv_canvas_create(lv_scr_act());
            lv_obj_center(image_canvas);
            free(canvas_buf);
            canvas_buf = jalloc(current_state->data.image.width * current_state->data.image.height * 2);

            lv_canvas_set_buffer(image_canvas, canvas_buf, current_state->data.image.width, current_state->data.image.height, LV_IMG_CF_TRUE_COLOR);
            lv_canvas_copy_buf(image_canvas, current_state->data.image.buf, 0, 0, current_state->data.image.width, current_state->data.image.height);

            lv_obj_invalidate(image_canvas);
            break;
        }
        }
        bsp_display_unlock();
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
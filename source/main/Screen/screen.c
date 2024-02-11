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
#include "../Camera/camera.h"
#include "../SYS_MODE/sys_mode.h"

#define TAG "screen"

void screen_task(void *arg)
{
    struct ScreenConf *conf = arg;

    struct meta_frame *held_mf = NULL;

    bool valid_qr_data = false;
    char qr_data[MAX_QR_SIZE] = {0};

    char state_text[MAX_QR_SIZE] = {0};
    lv_img_dsc_t *state_icon = NULL;

    char log_queue[10][30] = {0};
    int log_queue_index = 0;

    static lv_style_t style_bar_bg;

    lv_style_init(&style_bar_bg);
    lv_style_set_border_color(&style_bar_bg, lv_palette_main(LV_PALETTE_BLUE));
    lv_style_set_border_width(&style_bar_bg, 2);
    lv_style_set_pad_all(&style_bar_bg, 6); /*To make the indicator smaller*/
    lv_style_set_radius(&style_bar_bg, 6);
    lv_style_set_anim_time(&style_bar_bg, 1000);

    static lv_style_t bg_style;
    lv_style_set_bg_color(&bg_style, lv_color_white());
    lv_obj_add_style(lv_scr_act(), &bg_style, LV_PART_MAIN);

    static lv_style_t label_style;
    lv_style_set_text_color(&label_style, lv_color_black());

    lv_obj_t *state_bg = lv_img_create(lv_scr_act());
    lv_obj_align(state_bg, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t *state_lable = lv_label_create(lv_scr_act());
    lv_obj_set_width(state_lable, 150);
    lv_obj_align(state_lable, LV_ALIGN_CENTER, 0, 60);
    lv_obj_add_style(state_lable, &label_style, LV_PART_MAIN);

    lv_obj_t *qr_obj = lv_qrcode_create(lv_scr_act(), 240, lv_color_black(), lv_color_white());
    lv_obj_center(qr_obj);

    lv_obj_t *qr_err_lable = lv_label_create(lv_scr_act());
    lv_obj_set_width(qr_err_lable, 150);
    lv_obj_align(qr_err_lable, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_style(qr_err_lable, &label_style, LV_PART_MAIN);

    lv_obj_t *mirror_img = lv_img_create(lv_scr_act());
    int max_side = max(IMG_WIDTH, IMG_HEIGHT);
    float scale = 240.0 / (float)max_side;
    lv_img_set_zoom(mirror_img, (int)(255.0 * scale));
    lv_img_set_antialias(mirror_img, false); // Antialiasing destroys the image
    lv_obj_align(mirror_img, LV_ALIGN_CENTER, 0, 0);

    while (1)
    {

        struct ScreenMsg *msg;

        if (xQueueReceive(conf->to_screen_queue, &msg, get_rt_task_delay()) != pdPASS)
        {
            continue;
        }

        bsp_display_lock(0);

        lv_obj_add_flag(state_bg, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(state_lable, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(qr_obj, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(qr_err_lable, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(mirror_img, LV_OBJ_FLAG_HIDDEN);

        switch (msg->command)
        {
        case StateWarning:
        {
            memcpy(state_text, msg->data.text, MAX_QR_SIZE);
            state_icon = &warning;
            break;
        }
        case StateSuccess:
        {
            memcpy(state_text, msg->data.text, MAX_QR_SIZE);
            state_icon = &success;
            break;
        }
        case StateError:
        {
            memcpy(state_text, msg->data.text, MAX_QR_SIZE);
            state_icon = &failure;
            break;
        }
        case StateText:
        {
            memcpy(state_text, msg->data.text, MAX_QR_SIZE);
            state_icon = NULL;
            break;
        }
        case DrawQr:
        {
            memcpy(qr_data, msg->data.text, MAX_QR_SIZE);
            break;
        }
        case Mirror:
        {
            if (held_mf != NULL)
            {
                meta_frame_free(held_mf);
            }
            held_mf = msg->data.mf;
            break;
        }
        case PushLog:
        {
            memcpy(log_queue[log_queue_index], msg->data.text, 30);
            log_queue_index = (log_queue_index + 1) % 10;
            break;
        }
        }
        bsp_display_unlock();

        switch (get_mode())
        {
        case state_display:
        {
            if (state_icon != NULL)
            {
                lv_obj_clear_flag(state_bg, LV_OBJ_FLAG_HIDDEN);
                lv_img_set_src(state_bg, state_icon);
            }
            lv_obj_clear_flag(state_lable, LV_OBJ_FLAG_HIDDEN);
            lv_label_set_text_fmt(state_lable, "%s", state_text);

            break;
        }
        case log_queue_display:
        {

            // for (int i = 0; i < 10; i++)
            // {
            //     lv_obj_t *lable = lv_label_create(lv_scr_act());
            //     lv_label_set_text_fmt(lable, "%s", log_queue[i]);
            //     lv_obj_set_width(lable, 150);
            //     lv_obj_align(lable, LV_ALIGN_CENTER, 0, 24 * 1);

            //     lv_obj_add_style(lable, &label_style, LV_PART_MAIN);
            // }
            // break;
        }

        case qr_display:
        {
            if (valid_qr_data)
            {
                lv_obj_clear_flag(qr_obj, LV_OBJ_FLAG_HIDDEN);
                lv_qrcode_update(qr_obj, qr_data, strlen(qr_data));
            }
            else
            {
                lv_obj_clear_flag(qr_err_lable, LV_OBJ_FLAG_HIDDEN);
                lv_label_set_text_fmt(qr_err_lable, "%s", "No QR data");
            }

            break;
        }
        case mirror:
        {
            lv_obj_clear_flag(mirror_img, LV_OBJ_FLAG_HIDDEN);

            lv_img_dsc_t img = {
                .header.always_zero = 0,
                .header.cf = LV_IMG_CF_TRUE_COLOR,
                .header.w = IMG_WIDTH,
                .header.h = IMG_HEIGHT,
                .data_size = sizeof(held_mf->buf),
                .data = held_mf->buf,
            };

            lv_img_set_src(mirror_img, &img);

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
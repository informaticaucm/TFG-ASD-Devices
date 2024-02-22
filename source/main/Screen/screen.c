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

#define ALLOWED_AGE_FOR_QR 10
void screen_task(void *arg)
{

    bt_device_record_t *bt_device_history = jalloc(BT_DEVICE_HISTORY_SIZE * sizeof(bt_device_record_t));

    struct ScreenConf *conf = arg;

    struct meta_frame *held_mf = NULL;

    int qr_timestamp = 0;
    char qr_data[MAX_QR_SIZE] = {0};

    char state_text[MAX_QR_SIZE] = {0};
    lv_img_dsc_t *state_icon = NULL;

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

    lv_obj_t *mirror_img = lv_img_create(lv_scr_act());
    int max_side = max(IMG_WIDTH, IMG_HEIGHT);
    float scale = 240.0 / (float)max_side;
    lv_img_set_zoom(mirror_img, (int)(255.0 * scale));
    lv_img_set_antialias(mirror_img, false); // Antialiasing destroys the image
    lv_obj_align(mirror_img, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t *bt_table = lv_table_create(lv_scr_act());
    lv_obj_set_width(bt_table, 240);
    lv_obj_set_height(bt_table, 240);
    lv_obj_align(bt_table, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_style(bt_table, &label_style, LV_PART_MAIN);

    while (1)
    {

        struct ScreenMsg *msg;

        if (xQueueReceive(conf->to_screen_queue, &msg, get_rt_task_delay()) != pdPASS)
        {
            continue;
        }

        bsp_display_lock(0);

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
            qr_timestamp = time(0);
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
        case BTUpdate:
        {
            memcpy(bt_device_history, msg->data.bt_devices, BT_DEVICE_HISTORY_SIZE * sizeof(bt_device_record_t));
            break;
        }
        }

        free(msg);

        switch (get_mode())
        {
        case state_display:
        {
            lv_obj_add_flag(qr_obj, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(mirror_img, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(bt_table, LV_OBJ_FLAG_HIDDEN);

            if (state_icon != NULL)
            {
                lv_obj_clear_flag(state_bg, LV_OBJ_FLAG_HIDDEN);
                lv_img_set_src(state_bg, state_icon);
            }
            else
            {
                lv_obj_add_flag(state_bg, LV_OBJ_FLAG_HIDDEN);
            }
            lv_obj_clear_flag(state_lable, LV_OBJ_FLAG_HIDDEN);
            lv_label_set_text_fmt(state_lable, "%s", state_text);

            break;
        }
        case BT_list:
        {

            lv_obj_add_flag(state_bg, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(state_lable, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(qr_obj, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(mirror_img, LV_OBJ_FLAG_HIDDEN);

            lv_obj_clear_flag(bt_table, LV_OBJ_FLAG_HIDDEN);

            lv_table_set_col_cnt(bt_table, 2);
            lv_table_set_row_cnt(bt_table, BT_DEVICE_HISTORY_SIZE + 1);
            lv_table_set_col_width(bt_table, 0, 120);
            lv_table_set_col_width(bt_table, 1, 120);
            lv_table_set_cell_value(bt_table, 0, 0, "Name");
            lv_table_set_cell_value(bt_table, 0, 1, "Address");

            for (int i = 0; i < BT_DEVICE_HISTORY_SIZE; i++)
            {
                bt_device_record_t record = bt_device_history[i];
                lv_table_set_cell_value(bt_table, i + 1, 0, record.name);
                char address[18];
                sprintf(address, "%02x:%02x:%02x:%02x:%02x:%02x", record.address[0], record.address[1], record.address[2], record.address[3], record.address[4], record.address[5]);
                lv_table_set_cell_value(bt_table, i + 1, 1, address);
            }

            break;
        }

        case qr_display:
        {
            lv_obj_add_flag(state_bg, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(state_lable, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(mirror_img, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(bt_table, LV_OBJ_FLAG_HIDDEN);

            lv_obj_clear_flag(qr_obj, LV_OBJ_FLAG_HIDDEN);

            if (ALLOWED_AGE_FOR_QR > time(0) - qr_timestamp)

            {
                lv_qrcode_update(qr_obj, qr_data, strlen(qr_data));
            }
            else
            {
                char err[] = "no tenemos el totp todavía";
                lv_qrcode_update(qr_obj, err, sizeof(err));
            }

            break;
        }
        case mirror:
        {
            lv_obj_add_flag(state_bg, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(state_lable, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(qr_obj, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(bt_table, LV_OBJ_FLAG_HIDDEN);

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
        case button_test:
        {
            lv_obj_add_flag(qr_obj, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(mirror_img, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(bt_table, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(state_bg, LV_OBJ_FLAG_HIDDEN);

            lv_obj_clear_flag(state_lable, LV_OBJ_FLAG_HIDDEN);
            lv_label_set_text_fmt(state_lable, "pulse los 4 botones para continuar");

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
#define TAG "screen"
#include "esp_log.h"

#define ENABLE_SCREEN 1

#if ENABLE_SCREEN

#include <stdio.h>
#include <sys/param.h>
#include "esp_err.h"
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

#define ALLOWED_AGE_FOR_QR 10
#define FLASH_TIME 10

void screen_task(void *arg)
{
    int flash_timeout = 0;
    enum Icon flash_icon_code = 0;

    struct ScreenConf *conf = arg;

    struct meta_frame *held_mf = NULL;

    int qr_timestamp = 0;
    char qr_data[MAX_QR_SIZE] = {0};

    char msg_text[MAX_QR_SIZE] = {0};

    enum StarterState starter_state = NoQRConfig;

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

    lv_obj_t *bg_image = lv_img_create(lv_scr_act());
    lv_obj_align(bg_image, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t *msg_label = lv_label_create(lv_scr_act());
    lv_obj_set_width(msg_label, 150);
    lv_obj_align(msg_label, LV_ALIGN_CENTER, 0, 60);
    lv_obj_add_style(msg_label, &label_style, LV_PART_MAIN);

    lv_obj_t *qr_obj_full_screen = lv_qrcode_create(lv_scr_act(), 240, lv_color_black(), lv_color_white());
    lv_obj_center(qr_obj_full_screen);

    lv_obj_t *qr_obj_smaller = lv_qrcode_create(lv_scr_act(), 170, lv_color_black(), lv_color_white());
    lv_obj_center(qr_obj_smaller);

    lv_obj_t *mirror_img = lv_img_create(lv_scr_act());
    int max_side = max(IMG_WIDTH, IMG_HEIGHT);
    float scale = 240.0 / (float)max_side;
    lv_img_set_zoom(mirror_img, (int)(255.0 * scale));
    lv_img_set_antialias(mirror_img, false); // Antialiasing destroys the image
    lv_obj_align(mirror_img, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t *notification_textarea = lv_label_create(lv_scr_act());
    lv_obj_set_width(notification_textarea, 150);
    lv_obj_align(notification_textarea, LV_ALIGN_CENTER, 0, -100);
    lv_obj_add_style(notification_textarea, &label_style, LV_PART_MAIN);

    lv_obj_t *flash_img = lv_img_create(lv_scr_act());
    lv_obj_align(flash_img, LV_ALIGN_CENTER, 0, 0);

    while (1)
    {

        struct ScreenMsg *msg;

        if (xQueueReceive(conf->to_screen_queue, &msg, get_rt_task_delay()) == pdPASS)
        {

            switch (msg->command)
            {
            case StarterStateInform:
            {
                ESP_LOGE(TAG, "cambio de estado en la notificacion");
                starter_state = msg->data.starter_state;
                break;
            }
            case ShowMsg:
            {
                strncpy(msg_text, msg->data.text, sizeof(msg_text));
                set_tmp_mode(msg_display, 2, get_notmp_mode());
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
            case Flash:
            {
                ESP_LOGE(TAG, "STARTED FLASH WITH %d, until %d ", msg->data.icon, (int)time(0) + FLASH_TIME);
                flash_icon_code = msg->data.icon;
                flash_timeout = time(0) + FLASH_TIME;
            }
            }

            free(msg);
        }
        else
        {
            switch (get_mode())
            {

            case mirror:
            {
                break;
            }
            default:
            {
                vTaskDelay(get_task_delay());
                break;
            }
            }
        }

        bsp_display_lock(0);

        // ESP_LOGE(TAG, "time: %d, flash_timeout: %d, %d", (int)time(0), flash_timeout, time(0) < flash_timeout);

        if (time(0) < flash_timeout)
        {

            lv_obj_add_flag(qr_obj_full_screen, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(qr_obj_smaller, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(mirror_img, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(bg_image, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(msg_label, LV_OBJ_FLAG_HIDDEN);

            lv_obj_clear_flag(flash_img, LV_OBJ_FLAG_HIDDEN);

            switch (flash_icon_code)
            {
            case OK_Icon:
                lv_img_set_src(flash_img, &success);
                break;

            case NotFound_Icon:
                lv_img_set_src(flash_img, &failure);
                break;

            case OtherClass_Icon:
                lv_img_set_src(flash_img, &warning);
                break;
            }
        }
        else
        {
            lv_obj_add_flag(flash_img, LV_OBJ_FLAG_HIDDEN);

            if (starter_state != Success)
            {
                lv_obj_clear_flag(notification_textarea, LV_OBJ_FLAG_HIDDEN);

                char notification[100];

                char *stater_state_to_string[] = {
                    "NoQRConfig",
                    "NoWifi",
                    "NoAuth",
                    "NoTB",
                    "NoBackendAuth",
                    "NoBackend",
                    "Success"};

                snprintf(notification, sizeof(notification), "estado: %s", stater_state_to_string[starter_state]);

                lv_label_set_text(notification_textarea, notification);
            }
            else
            {
                lv_obj_add_flag(notification_textarea, LV_OBJ_FLAG_HIDDEN);
            }

            switch (get_mode())
            {

            case msg_display:
            {

                // ESP_LOGE(TAG, "msg_display %s", msg_text);

                lv_obj_add_flag(qr_obj_full_screen, LV_OBJ_FLAG_HIDDEN);
                lv_obj_add_flag(qr_obj_smaller, LV_OBJ_FLAG_HIDDEN);
                lv_obj_add_flag(mirror_img, LV_OBJ_FLAG_HIDDEN);
                lv_obj_add_flag(bg_image, LV_OBJ_FLAG_HIDDEN);

                lv_obj_clear_flag(msg_label, LV_OBJ_FLAG_HIDDEN);

                lv_label_set_text(msg_label, msg_text);

                break;
            }
            case qr_display:
            {

                // ESP_LOGE(TAG, "qr_display %s", qr_data);

                lv_obj_add_flag(bg_image, LV_OBJ_FLAG_HIDDEN);
                lv_obj_add_flag(msg_label, LV_OBJ_FLAG_HIDDEN);
                lv_obj_add_flag(mirror_img, LV_OBJ_FLAG_HIDDEN);

                lv_obj_t *used_qr = qr_obj_full_screen;
                lv_obj_t *unused_qr = qr_obj_smaller;

                if (starter_state != Success)
                {
                    unused_qr = qr_obj_full_screen;
                    used_qr = qr_obj_smaller;
                }

                lv_obj_add_flag(unused_qr, LV_OBJ_FLAG_HIDDEN);
                lv_obj_clear_flag(used_qr, LV_OBJ_FLAG_HIDDEN);

                if (ALLOWED_AGE_FOR_QR > time(0) - qr_timestamp)

                {
                    lv_qrcode_update(used_qr, qr_data, strlen(qr_data));
                }
                else
                {
                    char err[] = "no tenemos el totp todavía";
                    lv_qrcode_update(used_qr, err, sizeof(err));
                }

                break;
            }
            case mirror:
            {
                // ESP_LOGE(TAG, "mirror tick %p", held_mf);

                lv_obj_add_flag(bg_image, LV_OBJ_FLAG_HIDDEN);
                lv_obj_add_flag(msg_label, LV_OBJ_FLAG_HIDDEN);
                lv_obj_add_flag(qr_obj_full_screen, LV_OBJ_FLAG_HIDDEN);
                lv_obj_add_flag(qr_obj_smaller, LV_OBJ_FLAG_HIDDEN);

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

                lv_obj_add_flag(qr_obj_full_screen, LV_OBJ_FLAG_HIDDEN);
                lv_obj_add_flag(qr_obj_smaller, LV_OBJ_FLAG_HIDDEN);
                lv_obj_add_flag(mirror_img, LV_OBJ_FLAG_HIDDEN);
                lv_obj_add_flag(bg_image, LV_OBJ_FLAG_HIDDEN);

                lv_obj_clear_flag(msg_label, LV_OBJ_FLAG_HIDDEN);
                lv_label_set_text_fmt(msg_label, "pulse los 4 botones para continuar");

                break;
            }
            }

            bsp_display_unlock();
        }
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

#else

void screen_start(struct ScreenConf *conf)
{
    ESP_LOGE(TAG, "Screen is disabled");
}

#endif
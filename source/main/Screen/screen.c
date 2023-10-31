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
#include "qrcode_classifier.h"

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

        if (bsp_display_lock(10))
        {
            bsp_display_unlock();
        }

        free(msg);
    }
}

void screen_start(struct ScreenConf *conf)
{

    heap_caps_print_heap_info(MALLOC_CAP_DMA);

    bsp_display_start();
    bsp_display_backlight_on();

    bsp_display_lock(0);

    lv_obj_t *label1 = lv_label_create(lv_scr_act());
    lv_label_set_recolor(label1, true); /*Enable re-coloring by commands in the text*/
    lv_label_set_text(label1, "#0000ff Re-color# #ff00ff words# #ff0000 of a# label "
                              "and  wrap long text automatically.");
    lv_obj_set_width(label1, 150);
    lv_obj_align(label1, LV_ALIGN_CENTER, 0, -30);
    bsp_display_unlock();

    ESP_LOGI(TAG, "screen start created canvas successfuly");

    int err = xTaskCreate(&screen_task, "Screen task", 35000, conf, 1, NULL);
    if (err != pdPASS)
    {
        ESP_LOGE(TAG, "Problem on task start");
    }
}
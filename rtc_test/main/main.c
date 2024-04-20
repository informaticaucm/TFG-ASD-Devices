#include <stdio.h>

#define TAG "screen"
#include "esp_log.h"

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



void app_main(void)
{

    static lv_style_t label_style;
    lv_style_set_text_color(&label_style, lv_color_black());

    lv_obj_t *msg_label = lv_label_create(lv_scr_act());
    lv_obj_set_width(msg_label, 150);
    lv_obj_align(msg_label, LV_ALIGN_CENTER, 0, 60);
    lv_obj_add_style(msg_label, &label_style, LV_PART_MAIN);

    while (1)
    {
        bsp_display_lock(0);

        int t = time(NULL);

        char timer[100];

        snprintf(timer, 100, "Time: %d", t);

        lv_label_set_text(msg_label, timer);

        bsp_display_unlock();
    }
}



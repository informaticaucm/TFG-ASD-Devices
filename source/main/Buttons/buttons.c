#ifndef __BUTTONS_H__
#define __BUTTONS_H__

/* ADC1 Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <stdlib.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
// #include "esp_adc_cal.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_adc/adc_oneshot.h"
#include "buttons.h"
#include "esp_heap_caps.h"
#include "../SYS_MODE/sys_mode.h"
#include "bsp/esp-bsp.h"

// ADC Channels
#define ADC1_EXAMPLE_CHAN0 ADC_CHANNEL_0
// ADC Attenuation
#define ADC_EXAMPLE_ATTEN ADC_ATTEN_DB_11
// ADC Calibration
#if CONFIG_IDF_TARGET_ESP32
#define ADC_EXAMPLE_CALI_SCHEME ESP_ADC_CAL_VAL_EFUSE_VREF
#elif CONFIG_IDF_TARGET_ESP32S2
#define ADC_EXAMPLE_CALI_SCHEME ESP_ADC_CAL_VAL_EFUSE_TP
#elif CONFIG_IDF_TARGET_ESP32C3
#define ADC_EXAMPLE_CALI_SCHEME ESP_ADC_CAL_VAL_EFUSE_TP
#elif CONFIG_IDF_TARGET_ESP32S3
#define ADC_EXAMPLE_CALI_SCHEME ESP_ADC_CAL_VAL_EFUSE_TP_FIT
#endif

static const char *TAG = "ADC SINGLE";

struct button_adc_config_t
{
    int button_index;
    int min;
    int max;
    enum sys_mode mode;
};

struct button_adc_config_t adc_buttons[4] = {
    {1, 2800, 3000, qr_display},
    {2, 2250, 2450, state_display},
    {3, 300, 500, BT_list},
    {4, 850, 1050, mirror}};
int adc_button_num = 4;

static bool adc_calibration_init(adc_cali_handle_t *out_handle)
{
    esp_err_t ret = ESP_FAIL;
    adc_cali_handle_t handle = NULL;
    bool calibrated = false;

#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    ESP_LOGI(TAG, "calibration scheme version is %s", "Curve Fitting");
    adc_cali_curve_fitting_config_t cali_config = {
        .unit_id = ADC_UNIT_1,
        .atten = ADC_EXAMPLE_ATTEN,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    ret = adc_cali_create_scheme_curve_fitting(&cali_config, &handle);

#elif ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    ESP_LOGI(TAG, "calibration scheme version is %s", "Line Fitting");
    adc_cali_line_fitting_config_t cali_config = {
        .unit_id = ADC_UNIT_1,
        .atten = ADC_EXAMPLE_ATTEN,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    ret = adc_cali_create_scheme_line_fitting(&cali_config, &handle);
#endif

    *out_handle = handle;
    if (ret == ESP_OK)
    {
        calibrated = true;
        ESP_LOGI(TAG, "Calibration Success");
    }
    else if (ret == ESP_ERR_NOT_SUPPORTED)
    {
        ESP_LOGW(TAG, "calibration fail due to lack of eFuse bits");
    }
    else
    {
        ESP_LOGE(TAG, "Invalid arg");
    }

    return calibrated;
}

int presed_buttons = 0;

static void adc_button_task(void *arg)
{

    adc_oneshot_unit_handle_t adc1_handle;
    adc_oneshot_unit_init_cfg_t init_config1 = {
        .unit_id = ADC_UNIT_1,
        // .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle));

    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_EXAMPLE_ATTEN,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ADC1_EXAMPLE_CHAN0, &config));

    adc_cali_handle_t adc1_cali_handle;
    adc_calibration_init(&adc1_cali_handle);

    while (1)
    {

        if (is_ota_running())
        {
            vTaskDelay(get_task_delay());
            continue;
        }
        vTaskDelay(get_rt_task_delay());

        int voltage = 0;

        adc_oneshot_read(adc1_handle, ADC1_EXAMPLE_CHAN0, &voltage);

        for (int i = 0; i < adc_button_num; ++i)
        {
            if ((voltage >= adc_buttons[i].min) && (voltage <= adc_buttons[i].max))
            {
                bsp_led_set(BSP_LED_GREEN, true);

                int button_pressed = adc_buttons[i].button_index;


                if (get_mode() == button_test)
                {
                    ESP_LOGE(TAG, "Button %d pressed", button_pressed);
                    presed_buttons = presed_buttons | (1 << i);
                    if(presed_buttons == 0b1111)
                    {
                        set_mode(mirror);
                        presed_buttons = 0;
                    }
                    ESP_LOGE(TAG, "presed_buttons: %d", presed_buttons);

                }
                else 
                {
                    set_mode(adc_buttons[i].mode);

                    const char *string_mode[] = {
                        "mirror",
                        "qr_display",
                        "state_display",
                        "log_queue_display",
                    };

                    ESP_LOGI(TAG, "Button %d pressed -> changing to mode %s", button_pressed, string_mode[adc_buttons[i].mode]);
                }
                vTaskDelay(100 / portTICK_PERIOD_MS);
                bsp_led_set(BSP_LED_GREEN, false);
            }
        }
    }
}

void buttons_start(struct ButtonsConf *conf)
{
    TaskHandle_t handle = jTaskCreate(&adc_button_task, "button task", 3 * 1024, NULL, 1, MALLOC_CAP_SPIRAM);
    if (handle == NULL)
    {
        ESP_LOGE(TAG, "Problem on task start");
    }
}

#endif

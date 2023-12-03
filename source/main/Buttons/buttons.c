#include "buttons.h"
#include "../common.h"
#include "iot_button.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "../SYS_MODE/sys_mode.h"

#define TAG "buttons"

typedef enum
{
    BUTTON_IDLE = 0,
    BUTTON_MENU,
    BUTTON_PLAY,
    BUTTON_UP,
    BUTTON_DOWN
} ButtonName;

static void button_single_click_cb(void *arg, void *usr_data)
{
    ESP_LOGI(TAG, "BUTTON_SINGLE_CLICK");
}

void button_task(void *arg)

{
    struct ButtonsConf *conf = arg;
    adc_oneshot_unit_handle_t adc1_handle;
    adc_oneshot_unit_init_cfg_t init_config1 = {
        .unit_id = ADC_UNIT_1,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle));

    while (1)
    {
        if (is_ota_running())
        {
            vTaskDelay(get_task_delay());
            continue;
        }
        vTaskDelay(get_rt_task_delay());
        uint32_t voltage = 0;

        adc_oneshot_read(adc1_handle, ADC_CHANNEL_0, &voltage);

        if (voltage > 1500 && voltage < 1700)
        {
            ESP_LOGE(TAG, "+");
            set_mode(qr_display);
        }

        if (voltage > 3400 && voltage < 3600)
        {
            ESP_LOGE(TAG, "-");
            set_mode(mirror);
        }
    }
}

void buttons_start(struct ButtonsConf *conf)
{

    TaskHandle_t handle = jTaskCreate(&button_task, "button task", 50000, conf, 1, MALLOC_CAP_SPIRAM);
    if (handle == NULL)
    {
        ESP_LOGE(TAG, "Problem on task start");
    }
}

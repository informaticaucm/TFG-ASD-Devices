#include "totp.h"
#include "../common.h"
#include "../nvs_plugin.h"
#include "../SYS_MODE/sys_mode.h"
#include "../Screen/screen.h"
#include "../Starter/starter.h"

#include "esp_log.h"
#include <string.h>

#include "lib/cotp.c"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#define TAG "totp"

static void totp_task(void *arg)
{
    struct TOTPConf *conf = arg;

    while (1)
    {
        vTaskDelay(get_task_delay());

        if (is_ota_running())
        {
            ESP_LOGE(TAG, "OTA running, I sleep");
            continue;
        }

        if (get_mode() == qr_display)
        {

            struct ConnectionParameters parameters;
            j_nvs_get(nvs_conf_tag, &parameters, sizeof(struct ConnectionParameters));

            if (parameters.backend_info_valid)
            {
                time_t now;
                time(&now);

                char *secret = parameters.backend_info.totp_seed;
                int t0 = parameters.backend_info.totp_t0;

                struct ConnectionParameters parameters;
                j_nvs_get(nvs_conf_tag, &parameters, sizeof(struct ConnectionParameters));
                int totp = do_the_totp_thing(now - t0, secret, 60, 6);

                jsend(conf->to_screen_queue, ScreenMsg, {
                    msg->command = DrawQr;
                    snprintf(msg->data.text, sizeof(msg->data.text), "http://10.3.141.119:5500/formulario-end?totp=%06d&espacioId=%d&dispositivoId=%d", totp, parameters.qr_info.space_id, parameters.backend_info.device_id);
                });
            }
        }
    }
}

void start_totp(struct TOTPConf *conf)
{
    TaskHandle_t handle = jTaskCreate(&totp_task, "totp task", 25000, conf, 1, MALLOC_CAP_SPIRAM);
    if (handle == NULL)
    {
        ESP_LOGE(TAG, "Problem on task start ");
        heap_caps_print_heap_info(MALLOC_CAP_SPIRAM);
    }
}
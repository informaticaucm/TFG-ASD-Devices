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
        if (!is_totp_ready())
        {
            ESP_LOGE(TAG, "TOTP not ready");
            continue;
        }
        if (is_ota_running())
        {
            ESP_LOGE(TAG, "OTA running, I sleep");
            continue;
        }

        if (get_mode() == qr_display)
        {
            char url[MAX_QR_SIZE];

            {
                time_t now;
                time(&now);

                char secret[17];
                char url_template[URL_SIZE];
                int t0;

                get_TOTP_secret(secret);
                t0 = get_TOTP_t0();
                get_qr_url_template(url_template);

                int totp = do_the_totp_thing(now - t0, secret, 30, 6);
                snprintf(url, sizeof(url), url_template, totp);
            }

            {
                struct ScreenMsg *msg = jalloc(sizeof(struct ScreenMsg));

                msg->command = DrawQr;
                strcpy(msg->data.text, url);

                int res = xQueueSend(conf->to_screen_queue, &msg, 0);
                if (res == pdFAIL)
                {
                    free(msg);
                }
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
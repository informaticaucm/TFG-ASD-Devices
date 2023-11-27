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
        vTaskDelay(TASK_DELAY*4);

        if (get_mode() == qr_display)
        {

            char url[MAX_QR_SIZE] = {0};

            time_t now;

          
                time(&now);
               

            {
                ESP_LOGI(TAG, "time: %d", (int) now);

                int totp = do_the_totp_thing(now, "JBSWY3DPEHPK3PXP", 30, 6);
                ESP_LOGI(TAG, "totp is: %d", totp);

                struct ConfigurationParameters parameters;
                get_parameters(&parameters);

                snprintf(url, MAX_QR_SIZE, "https://%s/?totp=%d&device=%s", "la.url.de.helena.y.galdo.asd", totp, parameters.device_name);
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
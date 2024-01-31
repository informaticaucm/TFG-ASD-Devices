#include "starter.h"
#include "../common.h"
#include "connect_wifi.h"
#include "../nvs_plugin.h"
#include "../MQTT/mqtt.h"
#include "../Screen/screen.h"
#include "../SYS_MODE/sys_mode.h"

#define TAG "starter"

enum StarterState starterState = NoQRConfig;

int tries = 0;
int cooldown = 0;

char *state_string[] = {
    "NoQRConfig",
    "NoWifi",
    "NoAuth",
    "NoMQTT",
    "Success",
};

void setState(enum StarterState state, struct StarterConf *conf)
{
    tries = 0;
    starterState = state;

    ESP_LOGI(TAG, "starter state changed to %s", state_string[state]);

    struct ConnectionParameters parameters;
    j_nvs_get(nvs_conf_tag, &parameters, sizeof(struct ConnectionParameters));
    switch (state)
    {
    case NoQRConfig:
        parameters.qr_valid = false;
    case NoWifi:
        crash_wifi();
    case NoAuth:
        parameters.access_token_valid = false;
    case NoMQTT:
        struct MQTTMsg *msg = jalloc(sizeof(struct MQTTMsg));
        msg->command = Disconect;

        int res = xQueueSend(conf->to_mqtt_queue, &msg, 0);
        if (res == pdFAIL)
        {
            ESP_LOGE(TAG, "mesage send fail");
            free(msg);
        }
    default:
        break;
    }

    j_nvs_set(nvs_conf_tag, &parameters, sizeof(struct ConnectionParameters));
}

void starter_task(void *arg)
{
    struct StarterConf *conf = arg;

    while (1)
    {

        ESP_LOGE(TAG, "starter is on state %s with tries %d (%d)", state_string[starterState], tries, cooldown);

        switch (starterState)
        {
        case NoQRConfig:
            struct ConnectionParameters parameters;
            int err = j_nvs_get(nvs_conf_tag, &parameters, sizeof(struct ConnectionParameters));

            if (err == ESP_ERR_NVS_NOT_FOUND || !parameters.qr_valid)
            {

                struct ScreenMsg *msg = jalloc(sizeof(struct ScreenMsg));

                msg->command = DisplayError;
                strcpy(msg->data.text, "no starting configuration found, plase scan QR code");

                int res = xQueueSend(conf->to_screen_queue, &msg, 0);
                if (res == pdFAIL)
                {
                    ESP_LOGE(TAG, "mesage send fail");
                    free(msg);
                }
            }
            else
            {
                setState(NoWifi, conf);
            }
            break;
        case NoWifi:
        {
            {
                struct ScreenMsg *msg = jalloc(sizeof(struct ScreenMsg));

                msg->command = DisplayText;

                snprintf(msg->data.text, MAX_QR_SIZE, "starting configuration was found, conecting to wifi %s... (try number %d)", parameters.qr_info.wifi_ssid, tries);

                int res = xQueueSend(conf->to_screen_queue, &msg, 0);
                if (res == pdFAIL)
                {
                    ESP_LOGE(TAG, "mesage send fail");
                    free(msg);
                }
            }

            int wifi_err = connect_wifi(parameters.qr_info.wifi_ssid, parameters.qr_info.wifi_psw);

            if (wifi_err != ESP_OK)
            {

                tries += 1;
                // ASK if we should reset the device or the configuration
            }
            else
            {
                setState(NoAuth, conf);
            }
            break;
        }

        case NoAuth:
        {

            struct ConnectionParameters parameters;
            int err = j_nvs_get(nvs_conf_tag, &parameters, sizeof(struct ConnectionParameters));
            if (err == ESP_ERR_NVS_NOT_FOUND)
            {
                setState(NoQRConfig, conf);
                break;
            }

            if (!parameters.access_token_valid)
            {
                if (cooldown == 0)
                {
                    {
                        struct ScreenMsg *msg = jalloc(sizeof(struct ScreenMsg));

                        msg->command = DisplayText;

                        snprintf(msg->data.text, MAX_QR_SIZE, "first time handshake with %s (try number %d)", parameters.qr_info.thingsboard_url, tries);

                        int res = xQueueSend(conf->to_screen_queue, &msg, 0);
                        if (res == pdFAIL)
                        {
                            ESP_LOGE(TAG, "mesage send fail");
                            free(msg);
                        }
                    }

                    {
                        struct MQTTMsg *msg = jalloc(sizeof(struct MQTTMsg));
                        msg->command = DoProvisioning;
                        memcpy(msg->data.provisioning.broker_url, parameters.qr_info.mqtt_broker_url, URL_SIZE);
                        memcpy(msg->data.provisioning.device_name, parameters.qr_info.device_name, 50);
                        memcpy(msg->data.provisioning.provisioning_device_secret, parameters.qr_info.provisioning_device_secret, 21);
                        memcpy(msg->data.provisioning.provisioning_device_key, parameters.qr_info.provisioning_device_key, 21);
                        int res = xQueueSend(conf->to_mqtt_queue, &msg, 0);
                        if (res == pdFAIL)
                        {
                            ESP_LOGE(TAG, "mesage send fail");
                            free(msg);
                        }
                    }

                    tries++;
                    cooldown = 20;
                }
                else
                {
                    cooldown -= 1;
                }
            }
            else
            {
                setState(NoMQTT, conf);
            }

            break;
        }
        case NoMQTT:
        {

            struct ConnectionParameters parameters;
            int err = j_nvs_get(nvs_conf_tag, &parameters, sizeof(struct ConnectionParameters));
            if (err == ESP_ERR_NVS_NOT_FOUND)
            {
                setState(NoQRConfig, conf);
                break;
            }

            if (!is_mqtt_normal_operation())
            {
                if (cooldown == 0)
                {
                    {
                        struct ScreenMsg *msg = jalloc(sizeof(struct ScreenMsg));

                        msg->command = DisplayText;

                        snprintf(msg->data.text, MAX_QR_SIZE, "normal handshake with %s (try number %d)", parameters.qr_info.thingsboard_url, tries);

                        int res = xQueueSend(conf->to_screen_queue, &msg, 0);
                        if (res == pdFAIL)
                        {
                            ESP_LOGE(TAG, "mesage send fail");
                            free(msg);
                        }
                    }

                    {
                        struct MQTTMsg *msg = jalloc(sizeof(struct MQTTMsg));
                        msg->command = Start;
                        memcpy(msg->data.start.broker_url, parameters.qr_info.mqtt_broker_url, URL_SIZE);
                        memcpy(msg->data.start.access_token, parameters.access_token, 21);

                        int res = xQueueSend(conf->to_mqtt_queue, &msg, 0);
                        if (res == pdFAIL)
                        {
                            ESP_LOGE(TAG, "mesage send fail");
                            free(msg);
                        }
                    }

                    tries++;
                    cooldown = 20;
                }
                else
                {
                    cooldown -= 1;
                }
            }
            else
            {
                setState(Success, conf);
            }

            break;
        }

        default:
            break;
        }

        struct StarterMsg *msg;
        if (xQueueReceive(conf->to_starter_queue, &msg, get_task_delay()) != pdPASS)
        {
            continue;
        }

        char *starter_command_to_string[] = {
            "QrInfo",
            "AuthInfo",
            "InvalidateConfig",
            "PingLost",
        };

        ESP_LOGI(TAG, "starter received message %s", starter_command_to_string[msg->command]);

        switch (msg->command)
        {
        case QrInfo:
        {
            struct ConnectionParameters parameters;
            j_nvs_get(nvs_conf_tag, &parameters, sizeof(struct ConnectionParameters));
            parameters.qr_valid = true;
            memcpy(&parameters.qr_info, &msg->data.qr, sizeof(struct QRInfo));

            j_nvs_set(nvs_conf_tag, &parameters, sizeof(struct ConnectionParameters));

            break;
        }
        case AuthInfo:
        {
            struct ConnectionParameters parameters;
            j_nvs_get(nvs_conf_tag, &parameters, sizeof(struct ConnectionParameters));
            parameters.access_token_valid = true;
            memcpy(&parameters.access_token, &msg->data.access_token, 21);

            j_nvs_set(nvs_conf_tag, &parameters, sizeof(struct ConnectionParameters));

            break;
        }
        case InvalidateConfig:
            setState(NoQRConfig, conf);
            break;
        case PingLost:
            setState(NoMQTT, conf);
            break;
        }
    }
    ESP_LOGE(TAG, "starter task ended");
}

void start_starter(struct StarterConf *conf)
{
    TaskHandle_t handle = jTaskCreate(&starter_task, "Starter task", 6000, conf, 1, MALLOC_CAP_INTERNAL);
    if (handle == NULL)
    {
        ESP_LOGE(TAG, "Problem on task start ");
        heap_caps_print_heap_info(MALLOC_CAP_INTERNAL);
    }
}

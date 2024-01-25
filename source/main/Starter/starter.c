#include "starter.h"
#include "../common.h"
#include "connect_wifi.h"
#include "../nvs_plugin.h"
#include "../MQTT/mqtt.h"
#include "../Screen/screen.h"
#include "../SYS_MODE/sys_mode.h"

#define TAG "starter"

enum StarterState
{
    NoQRConfig,
    NoWifi,
    NoAuth,
    NoPing,
    Success,
};

enum StarterState starterState = NoQRConfig;

void state_hop(struct StarterConf *conf)
{

    if (err == ESP_ERR_NVS_NOT_FOUND || !parameters.valid)
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

        return;
    }

    ESP_LOGI(TAG, "-- parameters after store --");

    ESP_LOGI(TAG, "parameters field thingsboard_url %s", parameters.thingsboard_url);
    ESP_LOGI(TAG, "parameters field mqtt_broker_url %s", parameters.mqtt_broker_url);
    ESP_LOGI(TAG, "parameters field provisioning_done %d", parameters.provisioning_done);
    ESP_LOGI(TAG, "parameters field wifi_psw %s", parameters.wifi_psw);
    ESP_LOGI(TAG, "parameters field wifi_ssid %s", parameters.wifi_ssid);
    ESP_LOGI(TAG, "parameters field device_name %s", parameters.device_name);

    if (parameters.provisioning_done)
    {
        ESP_LOGI(TAG, "parameters field parameters.provisioning.done.access_token %s", parameters.provisioning.done.access_token);
    }
    else
    {
        ESP_LOGI(TAG, "parameters field parameters.provisioning.done.provisioning_device_key %s", parameters.provisioning.due.provisioning_device_key);
        ESP_LOGI(TAG, "parameters field parameters.provisioning.done.provisioning_device_secret %s", parameters.provisioning.due.provisioning_device_secret);
    }

    if (parameters.provisioning_done)
    {
        {
            struct ScreenMsg *msg = jalloc(sizeof(struct ScreenMsg));

            msg->command = DisplaySuccess;
            strcpy(msg->data.text, "wifi connected, starting mqtt client");

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
            memcpy(msg->data.start.broker_url, parameters.mqtt_broker_url, URL_SIZE);
            memcpy(msg->data.start.access_token, parameters.provisioning.done.access_token, 21);

            int res = xQueueSend(conf->to_mqtt_queue, &msg, 0);
            if (res == pdFAIL)
            {
                free(msg);
            }
        }
    }
    else
    {
        {
            struct ScreenMsg *msg = jalloc(sizeof(struct ScreenMsg));

            msg->command = DisplaySuccess;
            strcpy(msg->data.text, "wifi connected, starting thingsboard provisioning");

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
            memcpy(msg->data.provisioning.broker_url, parameters.mqtt_broker_url, URL_SIZE);
            memcpy(msg->data.provisioning.device_name, parameters.device_name, 50);
            memcpy(msg->data.provisioning.provisioning_device_secret, parameters.provisioning.due.provisioning_device_secret, 21);
            memcpy(msg->data.provisioning.provisioning_device_key, parameters.provisioning.due.provisioning_device_key, 21);

            int res = xQueueSend(conf->to_mqtt_queue, &msg, 0);
            if (res == pdFAIL)
            {
                free(msg);
            }
        }
    }
}

int tries = 0;
int cooldown = 0;
void starter_task(void *arg)
{
    struct StarterConf *conf = arg;

    while (1)
    {

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

                return;
            }
            else
            {
                tries = 20;
                setState(NoWifi);
            }
            break;
        case NoWifi:
        {
            {
                struct ScreenMsg *msg = jalloc(sizeof(struct ScreenMsg));

                msg->command = DisplayInfo;

                snprintf(msg->data.text, MAX_QR_SIZE, "starting configuration was found, conecting to wifi %s... (try number %d)", parameters.wifi_ssid, tries)

                    int res = xQueueSend(conf->to_screen_queue, &msg, 0);
                if (res == pdFAIL)
                {
                    ESP_LOGE(TAG, "mesage send fail");
                    free(msg);
                }
            }

            int wifi_err = connect_wifi(parameters.wifi_ssid, parameters.wifi_psw);

            if (wifi_err != ESP_OK)
            {

                int tryes += 1;
                if (tryes > 20)
                {
                    setState(NoQRConfig);
                }
            }
            else
            {
                setState(NoAuth);
            }
            break;
        }

        case NoAuth:
        {

            struct ConnectionParameters parameters;
            int err = j_nvs_get(nvs_conf_tag, &parameters, sizeof(struct ConnectionParameters));
            if (err == ESP_ERR_NVS_NOT_FOUND)
            {
                setState(NoQRConfig);
                break;
            }

            if (!parameters.access_token_valid)
            {
                if (cooldown == 0)
                {
                    {
                        struct ScreenMsg *msg = jalloc(sizeof(struct ScreenMsg));

                        msg->command = DisplayInfo;

                        snprintf(msg->data.text, MAX_QR_SIZE, "first time handshake with %s (try number %d)", parameters.thingsboard_url, tries);

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
                        memcpy(msg->data.provisioning.broker_url, parameters.mqtt_broker_url, URL_SIZE);
                        memcpy(msg->data.provisioning.device_name, parameters.device_name, 50);
                        memcpy(msg->data.provisioning.provisioning_device_secret, parameters.provisioning.due.provisioning_device_secret, 21);
                        memcpy(msg->data.provisioning.provisioning_device_key, parameters.provisioning.due.provisioning_device_key, 21);
                        msg->data.provisioning.onDoneFlag = &parameters.provisioning_done;
                        int res = xQueueSend(conf->to_mqtt_queue, &msg, 0);
                        if (res == pdFAIL)
                        {
                            ESP_LOGE(TAG, "mesage send fail");
                            free(msg);
                        }
                    }

                    tries++;
                    cooldown = 1000;
                }
                else
                {
                    cooldown -= 1;
                }
            }
            else
            {
                setState(NoPing);
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

        switch (msg->command)
        {
        case QrInfo:
            struct ConnectionParameters parameters;
            j_nvs_get(nvs_conf_tag, &parameters, sizeof(struct ConnectionParameters));
            parameters.qr_valid = true;
            memcpy(&parameters.qr_info, &msg->data.qr, sizeof(struct QRInfo));

            j_nvs_set(nvs_conf_tag, &parameters, sizeof(struct ConnectionParameters));

            break;
        case AuthInfo:
            struct ConnectionParameters parameters;
            j_nvs_get(nvs_conf_tag, &parameters, sizeof(struct ConnectionParameters));
            parameters.access_token_valid = true;
            memcpy(&parameters.access_token_valid, &msg->data.access_token, 21);

            j_nvs_set(nvs_conf_tag, &parameters, sizeof(struct ConnectionParameters));

            break;
        case InvalidateConfig:
            setState(NoQRConfig);
            break;
        }
    }
}

void setState(enum StarterState state)
{
    tries = 0;
    starterState = state;

    struct ConnectionParameters parameters;
    j_nvs_get(nvs_conf_tag, &parameters, sizeof(struct ConnectionParameters));
    switch (state)
    {
    case NoQRConfig:
        parameters.qr_valid = false;
    case NoWifi:
        esp_wifi_stop();
    case NoAuth
        parameters.auth_valid = false;

        default:
        break;
    }

    j_nvs_set(nvs_conf_tag, &parameters, sizeof(struct ConnectionParameters));
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

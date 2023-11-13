#include "starter.h"
#include "../common.h"
#include "connect_wifi.h"
#include "../nvs_plugin.h"
#include "../MQTT/mqtt.h"
#include "../Screen/screen.h"

#define TAG "starter"

#define nvs_tag "conf"

void start_sequence(struct StarterConf *conf)
{
    struct ConfigurationParameters parameters;
    int err = j_nvs_get(nvs_tag, &parameters, sizeof(struct ConfigurationParameters));
    if (err == ESP_ERR_NVS_NOT_FOUND)
    {
        struct ScreenMsg *msg = malloc(sizeof(struct ScreenMsg));

        msg->command = DisplayWarning;
        strcpy(msg->data.text, "no starting configuration found");

        int res = xQueueSend(conf->to_screen_queue, &msg, 0);
        if (res == pdFAIL)
        {
            ESP_LOGE(TAG, "mesage send fail");
            free(msg);
        }

        return;
    }

    {
        struct ScreenMsg *msg = malloc(sizeof(struct ScreenMsg));

        msg->command = DisplayWarning;
        strcpy(msg->data.text, "starting configuration was found, conecting to wifi     ");
        strcpy(msg->data.text + 51, parameters.wifi_ssid);

        int res = xQueueSend(conf->to_screen_queue, &msg, 0);
        if (res == pdFAIL)
        {
            ESP_LOGE(TAG, "mesage send fail");
            free(msg);
        }
    }
    ESP_LOGE(TAG, "wifi %s, %s", parameters.wifi_ssid, parameters.wifi_psw);
    int err = connect_wifi(parameters.wifi_ssid, parameters.wifi_psw);

    if(err != ESP_OK){
        
    }

    {
        struct ScreenMsg *msg = malloc(sizeof(struct ScreenMsg));

        msg->command = DisplayWarning;
        strcpy(msg->data.text, "wifi connected, starting thingsboard provisioning");

        int res = xQueueSend(conf->to_screen_queue, &msg, 0);
        if (res == pdFAIL)
        {
            ESP_LOGE(TAG, "mesage send fail");
            free(msg);
        }
    }

    if (parameters.provisioning_done)
    {
        {
            struct MQTTMsg *msg = jalloc(sizeof(msg));
            msg->command = Start;
            strcpy(msg->data.start.broker_url, parameters.mqtt_broker_url);
            strcpy(msg->data.start.access_tocken, parameters.provisioning.done.access_tocken);

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
            struct MQTTMsg *msg = jalloc(sizeof(msg));
            msg->command = DoProvisioning;
            strcpy(msg->data.provisioning.broker_url, parameters.mqtt_broker_url);
            strcpy(msg->data.provisioning.device_name, parameters.provisioning.due.device_name);
            strcpy(msg->data.provisioning.provisioning_device_secret, parameters.provisioning.due.provisioning_device_secret);
            strcpy(msg->data.provisioning.provisioning_device_key, parameters.provisioning.due.provisioning_device_key);

            int res = xQueueSend(conf->to_mqtt_queue, &msg, 0);
            if (res == pdFAIL)
            {
                free(msg);
            }
        }
    }
}

void starter_task(void *arg)
{
    struct StarterConf *conf = arg;

    start_sequence(conf);

    while (1)
    {
        // ESP_LOGI(TAG, "tick");

        struct StarterMsg *msg;
        if (xQueueReceive(conf->to_starter_queue, &msg, TASK_DELAY) != pdPASS)
        {
            continue;
        }

        struct ConfigurationParameters parameters;

        switch (msg->command)
        {
        case QrInfo:
            parameters.provisioning_done = false;
            strcpy(parameters.wifi_ssid, msg->data.qr.wifi_ssid);
            strcpy(parameters.wifi_psw, msg->data.qr.wifi_psw);
            strcpy(parameters.mqtt_broker_url, msg->data.qr.mqtt_broker_url);
            strcpy(parameters.provisioning.due.device_name, msg->data.qr.device_name);
            strcpy(parameters.provisioning.due.provisioning_device_key, msg->data.qr.provisioning_device_key);
            strcpy(parameters.provisioning.due.provisioning_device_secret, msg->data.qr.provisioning_device_secret);
            break;
        case ProvisioningInfo:
            j_nvs_get(nvs_tag, &parameters, sizeof(struct ConfigurationParameters));
            parameters.provisioning_done = true;
            strcpy(parameters.provisioning.done.access_tocken, msg->data.provisioning.access_tocken);
            break;
        }
        j_nvs_set(nvs_tag, &parameters, sizeof(struct ConfigurationParameters));
        esp_restart();
    }
}

void start_starter(struct StarterConf *conf)
{
    TaskHandle_t handle = jTaskCreate(&starter_task, "Starter task", 10000, conf, 1, MALLOC_CAP_INTERNAL);
    if (handle == NULL)
    {
        ESP_LOGE(TAG, "Problem on task start ");
        heap_caps_print_heap_info(MALLOC_CAP_INTERNAL);
    }
}
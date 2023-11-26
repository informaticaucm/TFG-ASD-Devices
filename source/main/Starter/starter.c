#include "starter.h"
#include "../common.h"
#include "connect_wifi.h"
#include "../nvs_plugin.h"
#include "../MQTT/mqtt.h"
#include "../Screen/screen.h"

#define TAG "starter"

void start_sequence(struct StarterConf *conf)
{
    struct ConfigurationParameters parameters;
    int err = j_nvs_get(nvs_conf_tag, &parameters, sizeof(struct ConfigurationParameters));

    if (err == ESP_ERR_NVS_NOT_FOUND || !parameters.valid)
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

    ESP_LOGI(TAG, "-- parameters after store --");

    ESP_LOGI(TAG, "parameters field mqtt_broker_url %s", parameters.mqtt_broker_url);
    ESP_LOGI(TAG, "parameters field provisioning_done %d", parameters.provisioning_done);
    ESP_LOGI(TAG, "parameters field wifi_psw %s", parameters.wifi_psw);
    ESP_LOGI(TAG, "parameters field wifi_ssid %s", parameters.wifi_ssid);
    ESP_LOGI(TAG, "parameters field device_name %s", parameters.device_name);

    if (parameters.provisioning_done)
    {
        ESP_LOGI(TAG, "parameters field parameters.provisioning.done.access_tocken %s", parameters.provisioning.done.access_tocken);
    }
    else
    {
        ESP_LOGI(TAG, "parameters field parameters.provisioning.done.provisioning_device_key %s", parameters.provisioning.due.provisioning_device_key);
        ESP_LOGI(TAG, "parameters field parameters.provisioning.done.provisioning_device_secret %s", parameters.provisioning.due.provisioning_device_secret);
    }

    {
        struct ScreenMsg *msg = malloc(sizeof(struct ScreenMsg));

        msg->command = DisplayWarning;
        strcpy(msg->data.text, "starting configuration was found, conecting to wifi ");
        strcpy(msg->data.text + 51, parameters.wifi_ssid);

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
        j_nvs_reset(nvs_conf_tag);
        esp_restart();
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
            struct MQTTMsg *msg = jalloc(sizeof(struct MQTTMsg));
            msg->command = Start;
            memcpy(msg->data.start.broker_url, parameters.mqtt_broker_url, URL_SIZE);
            memcpy(msg->data.start.access_tocken, parameters.provisioning.done.access_tocken, 21);

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
            ESP_LOGE(TAG, "wifi_ssid: %s", msg->data.qr.wifi_ssid);
            ESP_LOGE(TAG, "wifi_psw: %s", msg->data.qr.wifi_psw);
            parameters.provisioning_done = false;
            parameters.valid = true;

            strcpy(parameters.wifi_ssid, msg->data.qr.wifi_ssid);
            strcpy(parameters.wifi_psw, msg->data.qr.wifi_psw);
            strcpy(parameters.mqtt_broker_url, msg->data.qr.mqtt_broker_url);
            strcpy(parameters.device_name, msg->data.qr.device_name);
            strcpy(parameters.provisioning.due.provisioning_device_key, msg->data.qr.provisioning_device_key);
            strcpy(parameters.provisioning.due.provisioning_device_secret, msg->data.qr.provisioning_device_secret);
            break;
        case ProvisioningInfo:
            j_nvs_get(nvs_conf_tag, &parameters, sizeof(struct ConfigurationParameters));
            parameters.provisioning_done = true;
            parameters.valid = true;
            strcpy(parameters.provisioning.done.access_tocken, msg->data.provisioning.access_tocken);
            break;
        case UnvalidateConfig:
            parameters.valid = false;
            break;
        }
        ESP_LOGI(TAG, "-- parameters before store --");

        ESP_LOGI(TAG, "parameters field mqtt_broker_url %s", parameters.mqtt_broker_url);
        ESP_LOGI(TAG, "parameters field provisioning_done %d", parameters.provisioning_done);
        ESP_LOGI(TAG, "parameters field wifi_psw %s", parameters.wifi_psw);
        ESP_LOGI(TAG, "parameters field wifi_ssid %s", parameters.wifi_ssid);
        ESP_LOGI(TAG, "parameters field device_name %s", parameters.device_name);

        if (parameters.provisioning_done)
        {
            ESP_LOGI(TAG, "parameters field parameters.provisioning.done.access_tocken %s", parameters.provisioning.done.access_tocken);
        }
        else
        {
            ESP_LOGI(TAG, "parameters field parameters.provisioning.done.provisioning_device_key %s", parameters.provisioning.due.provisioning_device_key);
            ESP_LOGI(TAG, "parameters field parameters.provisioning.done.provisioning_device_secret %s", parameters.provisioning.due.provisioning_device_secret);
        }
        j_nvs_set(nvs_conf_tag, &parameters, sizeof(struct ConfigurationParameters));
        esp_restart();
    }
}

void start_starter(struct StarterConf *conf)
{
    TaskHandle_t handle = jTaskCreate(&starter_task, "Starter task", 25000, conf, 1, MALLOC_CAP_INTERNAL);
    if (handle == NULL)
    {
        ESP_LOGE(TAG, "Problem on task start ");
        heap_caps_print_heap_info(MALLOC_CAP_INTERNAL);
    }
}
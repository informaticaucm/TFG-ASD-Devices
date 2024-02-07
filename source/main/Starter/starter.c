#include "starter.h"
#include "../common.h"
#include "../nvs_plugin.h"
#include "../MQTT/mqtt.h"
#include "../Screen/screen.h"
#include "../SYS_MODE/sys_mode.h"

/* WiFi station Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"

#define TAG "starter"

esp_netif_t *my_ap = 0;
static const char *WIFI_TAG = "wifi station";
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

void crash_wifi()
{
    if (my_ap != 0)
    {
        esp_wifi_stop();
        esp_netif_destroy(my_ap);
        my_ap = 0;
    }
}

void setState(enum StarterState state, struct StarterConf *conf)
{
    if (starterState == state)
    {
        return;
    }

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
    case NoAuth:
    case NoTB:
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

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1

static void event_handler(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data)
{
    struct StarterConf *conf = arg;

    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(WIFI_TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

int connect_wifi(char *WIFI_SSID, char *WIFI_PASSWORD, struct StarterConf *conf)
{
    crash_wifi();
    s_wifi_event_group = xEventGroupCreate();

    my_ap = esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        conf,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler,
                                                        conf,
                                                        &instance_got_ip));

    wifi_config_t wifi_config = {0};

    strcpy((char *)wifi_config.sta.ssid, WIFI_SSID);
    strcpy((char *)wifi_config.sta.password, WIFI_PASSWORD);
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(WIFI_TAG, "wifi_init_sta finished.");

    /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
     * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                                           WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                           pdFALSE,
                                           pdFALSE,
                                           portMAX_DELAY);

    /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
     * happened. */
    if (bits & WIFI_CONNECTED_BIT)
    {
        ESP_LOGI(WIFI_TAG, "connected to ap SSID:%s password:%s",
                 WIFI_SSID, WIFI_PASSWORD);
    }
    else if (bits & WIFI_FAIL_BIT)
    {
        ESP_LOGI(WIFI_TAG, "Failed to connect to SSID:%s, password:%s",
                 WIFI_SSID, WIFI_PASSWORD);
    }
    else
    {
        ESP_LOGE(WIFI_TAG, "UNEXPECTED EVENT");
    }

    esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, instance_any_id);
    esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, instance_got_ip);

    return 0;
}

void print_ConnectionParameters(struct ConnectionParameters *cp)
{

    ESP_LOGI(TAG, "ConnectionParameters:");
    ESP_LOGI(TAG, "   qr_valid: %d", cp->qr_valid);
    ESP_LOGI(TAG, "      .wifi_ssid: %s", cp->qr_info.wifi_ssid);
    ESP_LOGI(TAG, "      .wifi_psw: %s", cp->qr_info.wifi_psw);
    ESP_LOGI(TAG, "      .thingsboard_url: %s", cp->qr_info.thingsboard_url);
    ESP_LOGI(TAG, "      .mqtt_broker_url: %s", cp->qr_info.mqtt_broker_url);
    ESP_LOGI(TAG, "      .device_name: %s", cp->qr_info.device_name);
    ESP_LOGI(TAG, "      .space_id: %d", cp->qr_info.space_id);
    ESP_LOGI(TAG, "      .provisioning_device_key: %s", cp->qr_info.provisioning_device_key);
    ESP_LOGI(TAG, "      .provisioning_device_secret: %s", cp->qr_info.provisioning_device_secret);
    ESP_LOGI(TAG, "   access_token_valid: %d", cp->access_token_valid);
    ESP_LOGI(TAG, "      access_token: %s", cp->access_token);
    ESP_LOGI(TAG, "   totp_seed_valid: %d", cp->totp_seed_valid);
    ESP_LOGI(TAG, "      totp_seed: %s", cp->totp_seed);
}

void starter_task(void *arg)
{
    struct StarterConf *conf = arg;

    while (1)
    {
        if (is_ota_running())
        {
            vTaskDelay(get_task_delay());
            continue;
        }

        if (starterState == Success)
        {
            vTaskDelay(get_idle_task_delay());
        }
        else
        {
            vTaskDelay(get_task_delay());
            ESP_LOGE(TAG, "starter is on state %s with tries %d (%d)", state_string[starterState], tries, cooldown);
            struct ConnectionParameters parameters;
            int err = j_nvs_get(nvs_conf_tag, &parameters, sizeof(struct ConnectionParameters));
            // print_ConnectionParameters(&parameters);
        }

        wifi_ap_record_t ap_info;
        if (esp_wifi_sta_get_ap_info(&ap_info) != ESP_OK)
        {
            ESP_LOGE(TAG, "wifi not connected");
            setState(NoWifi, conf);
        }

        switch (starterState)
        {
        case NoQRConfig:
        {
            struct ConnectionParameters parameters;
            int err = j_nvs_get(nvs_conf_tag, &parameters, sizeof(struct ConnectionParameters));

            if (err == ESP_ERR_NVS_NOT_FOUND || !parameters.qr_valid)
            {
                if (get_mode() == message)
                {
                    struct ScreenMsg *msg = jalloc(sizeof(struct ScreenMsg));

                    msg->command = DisplayText;
                    strcpy(msg->data.text, "no starting configuration found, plase scan QR code");

                    int res = xQueueSend(conf->to_screen_queue, &msg, 0);
                    if (res == pdFAIL)
                    {
                        ESP_LOGE(TAG, "mesage send fail");
                        free(msg);
                    }
                }
                tries++;
            }
            else
            {
                setState(NoWifi, conf);
            }
            break;
        }
        case NoWifi:
        {
            struct ConnectionParameters parameters;
            int err = j_nvs_get(nvs_conf_tag, &parameters, sizeof(struct ConnectionParameters));
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

            int wifi_err = connect_wifi(parameters.qr_info.wifi_ssid, parameters.qr_info.wifi_psw, conf);

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
                // ESP_LOGI(TAG, "no access token");
                // print_ConnectionParameters(&parameters);

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
                setState(NoTB, conf);
            }

            break;
        }
        case NoTB:
        {

            struct ConnectionParameters parameters;
            int err = j_nvs_get(nvs_conf_tag, &parameters, sizeof(struct ConnectionParameters));
            if (err == ESP_ERR_NVS_NOT_FOUND)
            {
                setState(NoQRConfig, conf);
                break;
            }

            ESP_LOGE(TAG, "last ping time %d, current time %d", get_last_ping_time(), (int)time(0));

            if (time(0) - get_last_tb_ping_time() > 10)
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

                if (tries > 10)
                {
                    parameters.access_token_valid = false;
                    j_nvs_set(nvs_conf_tag, &parameters, sizeof(struct ConnectionParameters));
                    setState(NoAuth, conf);
                }
            }
            else
            {
                setState(NoBackend, conf);
            }

            break;
        }
        case NoBackend:
        {

            struct ConnectionParameters parameters;
            int err = j_nvs_get(nvs_conf_tag, &parameters, sizeof(struct ConnectionParameters));
            if (err == ESP_ERR_NVS_NOT_FOUND)
            {
                setState(NoQRConfig, conf);
                break;
            }

            ESP_LOGE(TAG, "last ping time %d, current time %d", get_last_ping_time(), (int)time(0));

            if (time(0) - get_last_ping_time() > 10)
            {
                if (cooldown == 0)
                {
                    {
                        struct ScreenMsg *msg = jalloc(sizeof(struct ScreenMsg));

                        msg->command = DisplayText;

                        snprintf(msg->data.text, MAX_QR_SIZE, "normal handshake with backend (try number %d)", tries);

                        int res = xQueueSend(conf->to_screen_queue, &msg, 0);
                        if (res == pdFAIL)
                        {
                            ESP_LOGE(TAG, "mesage send fail");
                            free(msg);
                        }
                    }

                    {
                        struct MQTTMsg *msg = jalloc(sizeof(struct MQTTMsg));
                        msg->command = LogInToServer;
                        memcpy(msg->data.login.name, parameters.qr_info.device_name, 50);
                        memcpy(msg->data.login.space_id, parameters.qr_info.space_id, 21);

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

                if (tries > 10)
                {
                    parameters.access_token_valid = false;
                    j_nvs_set(nvs_conf_tag, &parameters, sizeof(struct ConnectionParameters));
                    setState(NoTB, conf);
                }
            }
            else
            {
                setState(Success, conf);
            }

            break;
        }

        case Success:
        {
            struct MQTTMsg *msg = jalloc(sizeof(struct MQTTMsg));

            msg->command = SendPingToServer;
            int res = xQueueSend(conf->to_mqtt_queue, &msg, 0);
            if (res == pdFAIL)
            {
                ESP_LOGE(TAG, "mesage send fail");
                free(msg);
            }

            if (get_last_ping_time() - time(0) > 50)
            {
                setState(NoBackend, conf);
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

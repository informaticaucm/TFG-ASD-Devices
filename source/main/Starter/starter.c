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
    "NoTB",
    "NoBackendAuth",
    "NoBackend",
    "Success"};

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
    tries = 0;
    cooldown = 0;
    starterState = state;

    ESP_LOGI(TAG, "starter state changed to %s", state_string[state]);

    jsend(conf->to_screen_queue, ScreenMsg, {
        msg->command = StarterStateInform;
        msg->data.starter_state = starterState;
    });

}

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1

static void event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
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

esp_err_t connect_wifi(char *WIFI_SSID, char *WIFI_PASSWORD, struct StarterConf *conf)
{

    if(strlen(WIFI_SSID) == 0){
        return ESP_ERR_INVALID_ARG;
    }

    crash_wifi();
    s_wifi_event_group = xEventGroupCreate();

    my_ap = esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

    heap_caps_print_heap_info(0);

    int err = esp_wifi_init(&cfg);

    if (err != ESP_OK)
    {
        return err;
    }

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

    // ESP_LOGI(WIFI_TAG, "wifi_init_sta finished.");

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

    return ESP_OK;
}

void print_ConnectionParameters(struct ConnectionParameters *cp)
{

    ESP_LOGI(TAG, "ConnectionParameters:");
    ESP_LOGI(TAG, "   qr_info_valid: %d", cp->qr_info_valid);
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
    ESP_LOGI(TAG, "   backend_info_valid: %d", cp->backend_info_valid);
    ESP_LOGI(TAG, "      device_id: %d", cp->backend_info.device_id);
    ESP_LOGI(TAG, "      totp_seed: %s", cp->backend_info.totp_seed);
    ESP_LOGI(TAG, "      totp_t0: %d", cp->backend_info.totp_t0);
}

void manage_state(bool (*is_next_state_ready)(),
                  void (*try_move_to_next_state)(struct StarterConf *),
                  void (*prepare_to_go_to_fail_state)(),
                  int (*backoff_function)(int),
                  enum StarterState next_state,
                  enum StarterState fail_state,
                  struct StarterConf *conf,
                  int max_tries,
                  int time_bettween_tries)
{

    // ESP_LOGI(TAG, "trying to connect to wifi");
    // struct ConnectionParameters parameters;
    // j_nvs_get(nvs_conf_tag, &parameters, sizeof(struct ConnectionParameters));

    // print_ConnectionParameters(&parameters);

    // ESP_LOGE(TAG, "tries: %d", tries);

    if (!is_next_state_ready())
    {

        if (cooldown == 0)
        {
            try_move_to_next_state(conf);

            tries++;
            cooldown = time_bettween_tries + backoff_function(tries);
            if (tries > max_tries)
            {
                prepare_to_go_to_fail_state();
                setState(fail_state, conf);
            }
        }
        else
        {
            cooldown -= 1;
            cooldown = cooldown < 0 ? 0 : cooldown;
        }
    }
    else
    {
        setState(next_state, conf);
    }
}

void try_connect_wifi(struct StarterConf *conf)
{
    ESP_LOGI(TAG, "trying to connect to wifi");
    struct ConnectionParameters parameters;
    j_nvs_get(nvs_conf_tag, &parameters, sizeof(struct ConnectionParameters));

    print_ConnectionParameters(&parameters);

    int res = connect_wifi(parameters.qr_info.wifi_ssid, parameters.qr_info.wifi_psw, conf);

    ESP_LOGE(TAG, "wifi connection returned %s", esp_err_to_name(res));
}

bool is_wifi_connected()
{
    wifi_ap_record_t ap_info;
    return esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK;
}

void try_tb_auth(struct StarterConf *conf)
{
    // ESP_LOGI(TAG, "trying to auth to tb");
    struct ConnectionParameters parameters;
    j_nvs_get(nvs_conf_tag, &parameters, sizeof(struct ConnectionParameters));

    jsend(conf->to_mqtt_queue, MQTTMsg, {
        msg->command = DoProvisioning;
        memcpy(msg->data.provisioning.broker_url, parameters.qr_info.mqtt_broker_url, URL_SIZE);
        memcpy(msg->data.provisioning.device_name, parameters.qr_info.device_name, 50);
        memcpy(msg->data.provisioning.provisioning_device_secret, parameters.qr_info.provisioning_device_secret, 21);
        memcpy(msg->data.provisioning.provisioning_device_key, parameters.qr_info.provisioning_device_key, 21);
    });
    ESP_LOGI(TAG, "sent DoProvisioning message");
}

bool is_tb_authenticated()
{
    struct ConnectionParameters parameters;
    j_nvs_get(nvs_conf_tag, &parameters, sizeof(struct ConnectionParameters));

    return parameters.access_token_valid;
}

bool is_backend_authenticated()
{
    struct ConnectionParameters parameters;
    j_nvs_get(nvs_conf_tag, &parameters, sizeof(struct ConnectionParameters));

    return parameters.backend_info_valid;
}

void try_tb_connect(struct StarterConf *conf)
{
    ESP_LOGI(TAG, "trying to connect to tb");

    struct ConnectionParameters parameters;
    j_nvs_get(nvs_conf_tag, &parameters, sizeof(struct ConnectionParameters));

    jsend(conf->to_mqtt_queue, MQTTMsg, {
        msg->command = Start;
        memcpy(msg->data.start.broker_url, parameters.qr_info.mqtt_broker_url, URL_SIZE);
        memcpy(msg->data.start.access_token, parameters.access_token, 21);
    });
}

void invalidate_tb_auth()
{
    struct ConnectionParameters parameters;
    j_nvs_get(nvs_conf_tag, &parameters, sizeof(struct ConnectionParameters));
    parameters.access_token_valid = false;
    j_nvs_set(nvs_conf_tag, &parameters, sizeof(struct ConnectionParameters));
}

void invalidate_backend_auth()
{
    struct ConnectionParameters parameters;
    j_nvs_get(nvs_conf_tag, &parameters, sizeof(struct ConnectionParameters));
    parameters.backend_info_valid = false;
    j_nvs_set(nvs_conf_tag, &parameters, sizeof(struct ConnectionParameters));
}

void invalidate_tb_ping()
{
    set_last_tb_ping_time(-1);
}

void invalidate_backend_auth_ping()
{
    set_last_ping_time(-1);
}

void dont() {}

bool is_tb_connected()
{
    // ESP_LOGI(TAG, "checking if tb is connected %d %d", (int)time(0), get_last_tb_ping_time());
    return get_last_tb_ping_time() != -1 && time(0) - get_last_tb_ping_time() < PING_RATE * 3;
}

void try_backend_auth(struct StarterConf *conf)
{

    struct ConnectionParameters parameters;
    j_nvs_get(nvs_conf_tag, &parameters, sizeof(struct ConnectionParameters));

    ESP_LOGI(TAG, "trying to connect to backend as %s id: %d", parameters.qr_info.device_name, parameters.qr_info.space_id);

    jsend(conf->to_mqtt_queue, MQTTMsg, {
        msg->command = LogInToServer;
        memcpy(msg->data.login.name, parameters.qr_info.device_name, 50);
        msg->data.login.space_id = parameters.qr_info.space_id;
    });
}

int constant_backoff(int tries)
{
    return 0;
}

int linear_backoff(int tries)
{
    return tries * 10;
}

int exponential_backoff(int tries)
{
    return tries * tries * 10;
}

bool is_backend_connected()
{
    return get_last_ping_time() != -1 && time(0) - get_last_ping_time() < 10;
}

void send_ping_to_backend(struct StarterConf *conf)
{
    // ESP_LOGI(TAG, "sending ping to backend");

    jsend(conf->to_mqtt_queue, MQTTMsg, {
        msg->command = SendPingToServer;
    });
}

bool is_qr_info_valid()
{
    struct ConnectionParameters parameters;
    int err = j_nvs_get(nvs_conf_tag, &parameters, sizeof(struct ConnectionParameters));
    return err == ESP_OK && parameters.qr_info_valid;
}

void starter_task(void *arg)
{
    {
        struct ConnectionParameters parameters;
        j_nvs_get(nvs_conf_tag, &parameters, sizeof(struct ConnectionParameters));

        print_ConnectionParameters(&parameters);
    }
    struct StarterConf *conf = arg;

    while (1)
    {
        if (is_ota_running())
        {
            ESP_LOGE(TAG, "OTA is running, starter task waits");
            vTaskDelay(get_idle_task_delay());
            continue;
        }

        if (starterState == Success)
        {
            vTaskDelay(100 * get_idle_task_delay());
        }
        else
        {
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            // ESP_LOGE(TAG, "starter is on state %s with tries %d (%d)", state_string[starterState], tries, cooldown);
        }

        // ESP_LOGI(TAG, "managing state: %s", state_string[starterState]);

      
        switch (starterState)
        {
        case NoQRConfig:
            manage_state(&is_qr_info_valid, &dont, &dont, &constant_backoff, NoWifi, NoQRConfig, conf, 10, 10);
            break;
        case NoWifi:
            manage_state(&is_wifi_connected, &try_connect_wifi, &dont, &constant_backoff, NoAuth, NoWifi, conf, 10, 10); // latches
            break;
        case NoAuth:
            manage_state(&is_tb_authenticated, &try_tb_auth, &dont, &constant_backoff, NoTB, NoWifi, conf, 10, 10);
            break;
        case NoTB:
            manage_state(&is_tb_connected, &try_tb_connect, &dont, &constant_backoff, NoBackendAuth, NoAuth, conf, 3, 10);
            break;
        case NoBackendAuth:
            manage_state(&is_backend_authenticated, &try_backend_auth, &invalidate_backend_auth_ping, &constant_backoff, NoBackend, NoAuth, conf, 10, PING_RATE);
            break;
        case NoBackend:
            manage_state(&is_backend_connected, &send_ping_to_backend, &dont, &constant_backoff, Success, NoTB, conf, 10, 10);
            break;
        case Success:
            manage_state(&is_backend_connected, &send_ping_to_backend, &invalidate_backend_auth_ping, &constant_backoff, Success, NoBackend, conf, 10, PING_RATE);
            break;

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
            "TBAuthInfo",
            "BackendInfo",
            "InvalidateConfig"};

        switch (msg->command)
        {
        case QrInfo:
        {
            struct ConnectionParameters parameters;
            j_nvs_get(nvs_conf_tag, &parameters, sizeof(struct ConnectionParameters));
            parameters.qr_info_valid = true;
            memcpy(&parameters.qr_info, &msg->data.qr.qr_info, sizeof(struct QRInfo));

            j_nvs_set(nvs_conf_tag, &parameters, sizeof(struct ConnectionParameters));

            print_ConnectionParameters(&parameters);

            invalidate_tb_ping();
            invalidate_backend_auth_ping();

            if (msg->data.qr.invalidate_thingsboard_auth)
            {
                invalidate_tb_auth();
            }

            if (msg->data.qr.invalidate_backend_auth)
            {
                invalidate_backend_auth();
            }

            starterState = NoQRConfig;

            break;
        }
        case TBAuthInfo:
        {
            struct ConnectionParameters parameters;
            j_nvs_get(nvs_conf_tag, &parameters, sizeof(struct ConnectionParameters));
            parameters.access_token_valid = true;
            memcpy(&parameters.access_token, &msg->data.access_token, 21);

            j_nvs_set(nvs_conf_tag, &parameters, sizeof(struct ConnectionParameters));

            break;
        }
        case BackendInfo:
        {
            struct ConnectionParameters parameters;
            j_nvs_get(nvs_conf_tag, &parameters, sizeof(struct ConnectionParameters));
            parameters.backend_info_valid = true;
            memcpy(&parameters.backend_info, &msg->data.backend_info, sizeof(struct BackendInfo));
            j_nvs_set(nvs_conf_tag, &parameters, sizeof(struct ConnectionParameters));

            break;
        }
        case InvalidateConfig:
            setState(NoQRConfig, conf);
            break;
        }

        free(msg);
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

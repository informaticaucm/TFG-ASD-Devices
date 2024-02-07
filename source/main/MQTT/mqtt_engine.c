#pragma once
#include "esp_random.h"
#include "mqtt.h"

esp_mqtt_client_handle_t client = 0;

bool specting_pong = false;
int pong_timeout_time = 0;

static const char *TAG = "mqtt";

void mqtt_listener(char *topic, char *msg, struct MQTTConf *conf)
{

    ESP_LOGI(TAG, "topic %s", topic);
    ESP_LOGI(TAG, "msg %s", msg);

    jparse_ctx_t jctx;
    int err = json_parse_start(&jctx, msg, strlen(msg));
    if (err != OS_SUCCESS)
    {
        ESP_LOGE(TAG, "ERROR ON JSON PARSE: %d", err);
    }

    if (strncmp(topic, "v1/devices/me/rpc/response/", strlen("v1/devices/me/rpc/response/")) == 0) // response to a device rpc request
    {
        char method[20];
        json_obj_get_string(&jctx, "method", method, 20);

        if (0 == strcmp(method, "tb_ping"))
        {
            set_last_tb_ping_time(time(0));
        }
        if (0 == strcmp(method, "ping"))
        {
            set_last_ping_time(time(0));

            bool failure = false;
            int epoch;
            int err = json_obj_get_object(&jctx, "response");
            if (err == OS_SUCCESS)
            {
                err = json_obj_get_int(&jctx, "epoch", &epoch);
                if (err == OS_SUCCESS)
                {
                    struct timeval now = {.tv_sec = epoch};
                    settimeofday(&now, NULL);
                    ESP_LOGI(TAG, "epoch is: %d", epoch);
                }
                else
                {
                    failure = true;
                }
            }
            else
            {
                failure = true;
            }
            json_obj_leave_object(&jctx);

            if (failure)
            {
                ESP_LOGE(TAG, "ERROR ON JSON KEY EXTRACTION: %d", err);
            }
        }

        if (strcmp(method, "dispositivos"))
        {
            json_obj_get_object(&jctx, "response");

            char totp_secret[17];
            int t0;

            json_obj_get_object(&jctx, "totpConfig");
            json_obj_get_string(&jctx, "secret", totp_secret, 17);
            json_obj_get_int(&jctx, "t0", &t0);

            set_TOTP_secret(totp_secret);
            set_TOTP_t0(t0);
            set_TOTP_ready(true);
        }
    }
    else if (strcmp(topic, "v1/devices/me/attributes") == 0)
    {

        char fw_title[30];
        int err = json_obj_get_string(&jctx, "fw_title", fw_title, 30);

        if (err == OS_SUCCESS)
        {
            char fw_url[URL_SIZE];
            int err = json_obj_get_string(&jctx, "fw_url", fw_url, URL_SIZE);

            if (err == OS_SUCCESS)
            {
                struct OTAMsg *msg = jalloc(sizeof(struct OTAMsg));
                msg->command = Update;
                strcpy(msg->url, fw_url);

                ESP_LOGI(TAG, "installing new firmware from: %s", msg->url);

                int res = xQueueSend(conf->to_ota_queue, &msg, 0);
                if (res != pdTRUE)
                {
                    free(msg);
                }
            }
            else
            {
                char fw_version[30];
                json_obj_get_string(&jctx, "fw_version", fw_version, 30);

                struct ConnectionParameters parameters;
                get_parameters(&parameters);

                struct OTAMsg *msg = jalloc(sizeof(struct OTAMsg));
                msg->command = Update;
                snprintf(msg->url, sizeof(msg->url), "%s/api/v1/%s/firmware/?title=%s&version=%s", parameters.qr_info.thingsboard_url, parameters.access_token, fw_title, fw_version);

                ESP_LOGI(TAG, "installing new firmware from: %s", msg->url);

                int res = xQueueSend(conf->to_ota_queue, &msg, 0);
                if (res != pdTRUE)
                {
                    free(msg);
                }
            }
        }
    }
    else if (strcmp(topic, "/provision/response") == 0)
    {
        /*{
            "status":"SUCCESS",
            "credentialsType":"ACCESS_TOKEN",
            "credentialsValue":"sLzc0gDAZPkGMzFVTyUY"
        }*/
        char access_token[21];

        int err = json_obj_get_string(&jctx, "credentialsValue", access_token, 21);

        if (err == OS_SUCCESS)
        {
            {
                struct StarterMsg *msg = jalloc(sizeof(struct StarterMsg));
                msg->command = AuthInfo;

                memcpy(msg->data.access_token, access_token, 21);

                ESP_LOGI(TAG, "access token is : %s", msg->data.access_token);

                int res = xQueueSend(conf->to_starter_queue, &msg, 0);
                if (res != pdTRUE)
                {
                    free(msg);
                }
            }
        }
        // else
        // {

        //     struct StarterMsg *msg = jalloc(sizeof(struct StarterMsg));
        //     msg->command = InvalidateConfig;
        //     ESP_LOGI(TAG, "config error, invalidating!");

        //     int res = xQueueSend(conf->to_starter_queue, &msg, 0);
        //     if (res != pdTRUE)
        //     {
        //         free(msg);
        //     }
        // }
    }
}

void mqtt_task(void *arg)
{
    struct MQTTConf *conf = arg;

    while (1)
    {
        struct MQTTMsg *msg;
        if (xQueueReceive(conf->to_mqtt_queue, &msg, get_task_delay()) != pdPASS)
        {
            continue;
        }

        char *mqtt_command_to_string[] = {
            "OTA_failure",
            "OTA_state_update",
            "Found_TUI_qr",
            "Start",
            "Disconect",
            "DoProvisioning"};

        ESP_LOGI(TAG, "a message %s was recieved at mqtt module", mqtt_command_to_string[msg->command]);

        switch (msg->command)
        {
        case OTA_failure:
            mqtt_send_ota_fail(msg->data.ota_failure.failure_msg);
            break;
        case OTA_state_update:
            mqtt_send_ota_status_report(msg->data.ota_state_update.ota_state);
            break;
        case Found_TUI_qr:
            char params[MAX_QR_SIZE + 90];
            char fw_version[32];
            get_version(fw_version);

            snprintf(params, sizeof(params), "{fw_version: \"%s\",qr_content: \"%s\"}", fw_version, msg->data.found_tui_qr.TUI_qr);

            mqtt_send_rpc("seguimiento", params);
            break;
        case DoProvisioning:
        {
            if (client != 0)
            {
                esp_mqtt_client_stop(client);
                esp_mqtt_client_destroy(client);
                client = 0;
            }

            ESP_LOGI(TAG, "doProvisioning conf");
            ESP_LOGI(TAG, "brocker url %s", msg->data.start.broker_url);
            ESP_LOGI(TAG, "device name %s", msg->data.provisioning.device_name);
            ESP_LOGI(TAG, "device key %s", msg->data.provisioning.provisioning_device_key);
            ESP_LOGI(TAG, "device secret %s", msg->data.provisioning.provisioning_device_secret);
            const esp_mqtt_client_config_t mqtt_cfg = {
                .broker = {
                    .address.uri = msg->data.provisioning.broker_url,
                    .verification.certificate = (const char *)server_cert_pem_start},
                .credentials = {
                    .username = "provision",
                }};

            client = esp_mqtt_client_init(&mqtt_cfg);
            ESP_ERROR_CHECK(esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, arg));

            ESP_ERROR_CHECK(esp_mqtt_client_start(client));

            mqtt_subscribe("/provision/response");

            /*{
                "deviceName": "DEVICE_NAME",
                "provisionDeviceKey": "PUT_PROVISION_KEY_HERE",
                "provisionDeviceSecret": "PUT_PROVISION_SECRET_HERE"
            }*/

            char msg_buffer[200];
            snprintf(msg_buffer, sizeof(msg_buffer), "{"
                                                     "\"deviceName\": \"%s_%d\","
                                                     "\"provisionDeviceKey\": \"%s\","
                                                     "\"provisionDeviceSecret\": \"%s\""
                                                     "}",
                     msg->data.provisioning.device_name,
                     (int)esp_random(),
                     msg->data.provisioning.provisioning_device_key,
                     msg->data.provisioning.provisioning_device_secret);

            mqtt_send("/provision/request", msg_buffer);
        }
        break;
        case Start:
        {
            if (client != 0)
            {
                esp_mqtt_client_stop(client);
                esp_mqtt_client_destroy(client);
                client = 0;
            }

            ESP_LOGI(TAG, "start conf");
            ESP_LOGI(TAG, "brocker url %s", msg->data.start.broker_url);
            ESP_LOGI(TAG, "access_token %s", msg->data.start.access_token);

            const esp_mqtt_client_config_t mqtt_cfg = {
                .broker = {
                    .address.uri = msg->data.start.broker_url,
                    .verification.certificate = (const char *)server_cert_pem_start},
                .credentials = {
                    .username = msg->data.start.access_token,
                }};

            client = esp_mqtt_client_init(&mqtt_cfg);
            ESP_ERROR_CHECK(esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, arg));
            ESP_ERROR_CHECK(esp_mqtt_client_start(client));
            mqtt_subscribe("v1/devices/me/attributes");
            mqtt_subscribe("v1/devices/me/rpc/request/+");

            if (conf->send_updated_mqtt_on_start)
            {
                struct OTAMsg *msg = jalloc(sizeof(struct OTAMsg));
                msg->command = CancelRollback;
                int res = xQueueSend(conf->to_ota_queue, &msg, 0);
                if (res != pdTRUE)
                {
                    free(msg);
                }

                mqtt_send_ota_status_report(UPDATED);
            }

            {
                mqtt_send_rpc("tb_ping", "");

                break;
            }

            break;
        }
        case LogInToServer:
        {
            char params[80];

            snprintf(params, sizeof(params), "{\"nombre\":\"%s\",\"espacioId\":%d}", msg->data.login.name, msg->data.login.space_id);

            mqtt_send_rpc("devices", params);

            break;
        }
        case SendPingToServer:
        {
            mqtt_send_rpc("ping", "");
            break;
        }

        case Disconect:
        {

            printf("disconecting %d\n", (int)client);
            if (client != 0)
            {
                esp_mqtt_client_stop(client);
                esp_mqtt_client_destroy(client);
            }
            set_mqtt_normal_operation(false);
            client = 0;
            break;
        }
        }

        free(msg);
    }
}
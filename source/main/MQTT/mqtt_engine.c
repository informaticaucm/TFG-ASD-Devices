#pragma once
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
            bool failure = false;
            long long int epoch;
            int err = json_obj_get_object(&jctx, "response");
            if (err == OS_SUCCESS)
            {
                err = json_obj_get_int64(&jctx, "epoch", &epoch);
                if (err == OS_SUCCESS)
                {
                    struct timeval now = {.tv_sec = epoch};
                    settimeofday(&now, NULL);
                    ESP_LOGI(TAG, "epoch is: %lld", epoch);
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

            set_last_ping_time(time(0));
        }
        if (0 == strcmp(method, "dispositivos"))
        {
            json_obj_get_object(&jctx, "response");

            char totp_secret[17];
            int t0;

            json_obj_get_object(&jctx, "totpConfig");
            json_obj_get_string(&jctx, "secret", totp_secret, 17);
            json_obj_get_int(&jctx, "t0", &t0);

            struct StarterMsg *msg = jalloc(sizeof(struct StarterMsg));
            msg->command = TOTPInfo;

            msg->data.totp.totp_t0 = t0;
            memcpy(msg->data.totp.totp_seed, totp_secret, 17);

            int res = xQueueSend(conf->to_starter_queue, &msg, 0);
            if (res != pdTRUE)
            {
                free(msg);
            }
        }
        if (0 == strcmp(method, "ble"))
        {
            struct ConnectionParameters parameters;
            get_parameters(&parameters);

            /*
                {
                    "usuarioId": 1,
                    "nombre": "Grace",
                    "apellidos": "Hopper",
                    "macs": [
                        "00:11:22:33:FF:EE",
                        "11:22:33:44:AA:BB"
                    ]
                }
            */
            json_obj_get_object(&jctx, "response");
            json_obj_get_array(&jctx, "macs");

            int mac_count = json_arr_get_len(&jctx);
            for (int i = 0; i < mac_count; i++)
            {
                char mac[18];
                char decoded_mac[6];
                json_arr_get_string(&jctx, i, mac, 18);
                sscanf(mac, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", &decoded_mac[0], &decoded_mac[1], &decoded_mac[2], &decoded_mac[3], &decoded_mac[4], &decoded_mac[5]);

                for (int i = 0; i < BT_DEVICE_HISTORY_SIZE; i++)
                {
                    if (memcmp(device_history[i].address, decoded_mac, 6) == 0 && VALID_ENTRY(device_history[i]))
                    {
                        /*
                           {
                                "tipo_registro": "RegistroSeguimientoDispositivoBle",
                                "espacioId": 1,
                                "mac": "00:11:22:33:FF:EE"
                            }
                        */
                        char params[100];
                        snprintf(params, sizeof(params), "{"
                                                         "  \"tipo_registro\": \"RegistroSeguimientoDispositivoBle\","
                                                         "  \"espacioId\": %d,"
                                                         "  \"mac\": \"%s\","
                                                         "}",
                                 parameters.qr_info.space_id, parameters.qr_info.device_id, mac);

                        send_api_post("seguimiento", params);

                            break;
                    }
                }
            }
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
                msg->command = TBAuthInfo;

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
            "LogInToServer",
            "DoProvisioning",
            "SendPingToServer",
        };

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
            /*
            {
                "tipo_registro": "RegistroSeguimientoDispositivoQr",
                "espacioId": 1,
                "dispositivoId": 2,
                "qr": "ZXlKaGJHY2lPaUpTVTBFdFQwRkZVQ0lzSW1WdVl5STZJa0V5TlRaRFFrTXRTRk0xTVRJaUxDSnJhV1FpT2lJeE9HSXhZMlkzTlRoak1XUTBaV00yWW1SaE5qVTRPVE0xTjJGaVpHUTROU0lzSW5SNWNDSTZJa3BYVkNJc0ltTjBlU0k2SWtwWFZDSjkuZ0NieFA3OG8zRGdwRFRVUWJ1SG5pdUdnWXBBVHFnR2tSR3k3cGFDNmhScno3TjdlSWE2c0FPV0RPOUZobmotYzhvY01sNGNGNEpiX212NXFSUENoOXI1N1BCcXg3ak9oTUlNUFR3SkdwamN5QmFxdEhsWmx1MXZ1cFk1dFEzWTJqR3oxVGk0Qm55d2FlRUhQeUlQUUp0TjdGN2hJQU9Semo3SVk0c0lLa1ZYdFFKWmdhS1c4cEVIcV9HQ3FqOGk1YWFpTTB1Sm5SRzNHT2gzbGl2cDlOcGp2OWRvcXAzZ3lQYTF6anJnMkgxUnNPR24wajJRTUd2dHVWZmt1TndGLVNvUEtGRUN5SE9xMFpLMW9IMnNUTzgtSnd2SGZsYklaUXI1eFdUcFM4cTdNYlVYRXVxVVJ0cmcwVGotMno2dGRhT0xUNGIzVWVEdWZLMmFyM2JCZlJENC1uUkFMdG9ZMGVrY015R0ZPUzdvMU14bDNoeTVzSUctRXlTeVdldUJWeTY4YURXRHBpOXFab1F1WTFUYnh4YWtqbmNDT0d1X0doMWwxbV9tSzJsX0lkeVhDVF9HQ2Z6RnE0WlRrUFo1ZXlkTkJBUFp1eEJMVWI0QnJNYjVpRGRaalQ3QWdHT2xScmVfd0lSSG1tS204VzluRGVRUVJtYklYTzIzSnVPdzkuQkRDYXJmcTJyX1VrOERITmZzTndTUS40RHVReDFjZkpYYWRIbnVkclZhQnNzNDV6eHlkNmlvdXVTelpVeU9lTTRpa0ZfN2hET2d3bWFDbWEtWjk3X1FaQko1RHpWbjlTSmhLVVRBcXBWUjNCUkdBeEpfSEFYVTVqYVRqWHFidlVheHNoN1o1VGdaOWVjazBGSW9lMWxrd3Y1MXhFdllxcVFfWG9qcjRNQUVtTHVNRV85QXJDSzltTmFNQURJek9qNFZvUXRhRFAxbDI2eXRvY2Mtb0VOaWZCUllHdTI4TGJKTGt5UUt6eVF5NkZ1QU90V2pMTTBXQ1hWNy1vX2R2ajZxZmVZSE5CRDdZQlN4eXFkZ0Q4ZGN4TUJOZDJzSzczWXNaUEhFYTBWMS04eno3aG0zYkgzdFplbHB3UFdTY3FMTFdfU1VINTg2YzBGVmVJNmdndnF6amZMWl9ZNmVRaWJWU2RYZk90SkJrMjJRckxzdUNYYlJLOEcxdzl0MjNQd3U4dWtVQXc0djBsN0hlYVdfMFNKeUtTUFFBTlJQODNNeUZiSzdmbXpUWWFXOVRZTjJKcktOLVBMcGQyZElGU20yR2FfRWZhQ3dOSkJtNFJETXpETnJm\n",
                "totp": {
                    "value": 14050471,
                    "time": 1111111111
                }
            }
            */
            char params[MAX_QR_SIZE + 90];
            char fw_version[32];
            get_version(fw_version);

            snprintf(params, sizeof(params), "{fw_version: \"%s\",qr_content: \"%s\"}", fw_version, msg->data.found_tui_qr.TUI_qr);

            send_api_post("seguimiento", params);
            break;
        case DoProvisioning:
        {
            ESP_LOGI(TAG, "doProvisioning client %d", (int)client);

            if (client != 0)
            {
                ESP_LOGI(TAG, "doProvisioning stopping client");
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
                                                     "\"deviceName\": \"%s\","
                                                     "\"provisionDeviceKey\": \"%s\","
                                                     "\"provisionDeviceSecret\": \"%s\""
                                                     "}",
                     msg->data.provisioning.device_name,
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
                mqtt_send_rpc("tb_ping", "{}");
                break;
            }

            break;
        }
        case LogInToServer:
        {
            char *request_body = alloca(strlen(msg->data.login.name) + 80);

            snprintf(request_body, strlen(msg->data.login.name) + 80, "{\"nombre\":\"%s\",\"espacioId\":%d,\"idExternoDispositivo\":\"<th_id>\"}", msg->data.login.name, msg->data.login.space_id);

            send_api_post("dispositivos", request_body);

            break;
        }
        case SendPingToServer:
        {
            send_api_post("ping", "{}");
            break;
        }
        case FetchBTMacs:
        {
            char *request_body = alloca(50);
            snprintf(request_body, 50, "{\"espacioId\":%d}", msg->data.fetch_btmacs.space_id);
            send_api_post("ble", request_body);
            break;
        }
        }
        free(msg);
    }
}
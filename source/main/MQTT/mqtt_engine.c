#pragma once
#include "mqtt.h"
#include "nvs_plugin.h"
#include "esp_crt_bundle.h"
#include "tb_shared_attribute_ingest.c"
#include "tb_rpc_ingest.c"

esp_mqtt_client_handle_t client = 0;

bool specting_pong = false;
int pong_timeout_time = 0;
enum StarterState starter_state = NoQRConfig;

void mqtt_listener(char *topic, char *mqtt_msg, struct MQTTConf *conf)
{

    ESP_LOGI(TAG, "recieved %s (%d) through topic %s", mqtt_msg, strlen(mqtt_msg), topic);

    jparse_ctx_t jctx;

    int err = json_parse_start(&jctx, mqtt_msg, strlen(mqtt_msg));
    if (err != OS_SUCCESS)
    {
        ESP_LOGE(TAG, "ERROR ON JSON PARSE: %d", err);
    }

    if (strncmp(topic, "v1/devices/me/rpc/response/", 27) == 0)
    {
        ESP_LOGE(TAG, "rpc ingest (%s)", mqtt_msg);
        tb_rpc_ingest(&jctx, conf);
    }
    else if (strncmp(topic, "v1/devices/me/attributes/response/", 34) == 0)
    {
        json_obj_get_object(&jctx, "shared");
        tb_shared_attribute_ingest(&jctx, conf, true);
    }
    else if (strcmp(topic, "v1/devices/me/attributes") == 0)
    {
        tb_shared_attribute_ingest(&jctx, conf, false);
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
            jsend(conf->to_starter_queue, StarterMsg, {
                msg->command = TBAuthInfo;
                memcpy(msg->data.access_token, access_token, 21);
                ESP_LOGI(TAG, "access token is : %s", msg->data.access_token);
            });
        }
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

            if (starter_state == Success)
            {

                char params[MAX_QR_SIZE + 90];
                char fw_version[32];
                get_version(fw_version);

                snprintf(params, sizeof(params), "{fw_version: \"%s\",qr_content: \"%s\"}", fw_version, msg->data.found_tui_qr.TUI_qr);

                send_api_post("seguimiento", params);
            }
            else
            {
                jsend(conf->to_starter_queue, StarterMsg, {
                    msg->command = NotifyMalfunction;
                });
            }
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
                    .verification.crt_bundle_attach = esp_crt_bundle_attach,
                },
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
                    .verification.crt_bundle_attach = esp_crt_bundle_attach,
                },
                .credentials = {
                    .username = msg->data.start.access_token,
                }};

            client = esp_mqtt_client_init(&mqtt_cfg);
            ESP_ERROR_CHECK(esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, arg));
            ESP_ERROR_CHECK(esp_mqtt_client_start(client));
            mqtt_subscribe("v1/devices/me/attributes");
            mqtt_subscribe("v1/devices/me/rpc/request/+");
            mqtt_subscribe("v1/devices/me/attributes/response/+");

            if (conf->send_updated_mqtt_on_start)
            {
                jsend(conf->to_ota_queue, OTAMsg, {
                    msg->command = CancelRollback;
                });

                mqtt_send_ota_status_report(UPDATED);
            }

            mqtt_send_rpc("tb_ping", "{}");
            mqtt_ask_for_atributes();

            break;
        }
        case LogInToServer:
        {
            char *request_body = alloca(strlen(msg->data.login.name) + 80);

            snprintf(request_body, strlen(msg->data.login.name) + 80, "{\"nombre\":\"%s\",\"espacioId\":%d,\"idExternoDispositivo\":\"%s\"}", msg->data.login.name, msg->data.login.space_id, msg->data.login.name);

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
            if (starter_state != Success)
            {
                jsend(conf->to_starter_queue, StarterMsg, {
                    msg->command = NotifyMalfunction;
                });
                break;
            }
            else
            {
                char *request_body = alloca(50);
                snprintf(request_body, 50, "{\"espacioId\":%d}", msg->data.fetch_btmacs.space_id);
                send_api_post("ble", request_body);
                break;
            }
        }
        case TagScanned:
        {
            /*
                {
                    "tipo_registro": "RegistroSeguimientoDispositivoNFC",
                    "espacioId": 1,
                    "uid": "y7w1T/D23w=="
                }
            */

            if (starter_state != Success)
            {
                jsend(conf->to_starter_queue, StarterMsg, {
                    msg->command = NotifyMalfunction;
                });
                break;
            }
            else
            {
                struct ConnectionParameters parameters;
                ESP_ERROR_CHECK(j_nvs_get(nvs_conf_tag, &parameters, sizeof(struct ConnectionParameters)));

                char *request_body = alloca(500);

                snprintf(request_body, 500, "{\"uid\":%lld,\"espacioId\":%d, \"dispositivoId\": %d, \"tipo_registro\": \"RegistroSeguimientoDispositivoNFC\"}", msg->data.tag_scanned.sn, parameters.qr_info.space_id, parameters.backend_info.device_id);

                send_api_post("seguimiento", request_body);
            }
            break;
        }
        case StarterStateInformToMQTT:
        {
            starter_state = msg->data.starter_state;
        }
        }
        free(msg);
    }
}
#include "mqtt.h"

void tb_rpc_ingest(jparse_ctx_t *jctx, struct MQTTConf *conf)
{
    char method[20];
    json_obj_get_string(jctx, "method", method, 20);

    if (0 == strcmp(method, "tb_ping"))
    {
        set_last_tb_ping_time(time(0));
    }

    if (json_obj_get_object(jctx, "response") != OS_SUCCESS)
    {
        ESP_LOGI(TAG, "no response object found");
        return;
    }

    if (0 == strcmp(method, "ping"))
    {
        long long int epoch;

        if (json_obj_get_int64(jctx, "epoch", &epoch) == OS_SUCCESS)
        {
            struct timeval now = {.tv_sec = epoch};
            settimeofday(&now, NULL);
            ESP_LOGI(TAG, "epoch is: %lld", epoch);
        }

        set_last_ping_time(time(0));
    }
    else if (0 == strcmp(method, "dispositivos"))
    {
        char totp_secret[17];
        int t0;
        int id;

        if (json_obj_get_int(&jctx, "id", &id) == OS_SUCCESS &&
            json_obj_get_object(&jctx, "totpConfig") == OS_SUCCESS &&
            json_obj_get_string(&jctx, "secret", totp_secret, 17) == OS_SUCCESS &&
            json_obj_get_int(&jctx, "t0", &t0) == OS_SUCCESS)
        {
            jsend(conf->to_starter_queue, StarterMsg, {
                msg->command = BackendInfo;
                msg->data.backend_info.totp_t0 = t0;
                msg->data.backend_info.device_id = id;
                memcpy(msg->data.backend_info.totp_seed, totp_secret, 17);
            });
        }
    }
    else if (0 == strcmp(method, "seguimiento"))
    {
        int feedback_code = 0;

        if (json_obj_get_int(&jctx, "feedback_code", &feedback_code) != OS_SUCCESS)
        {
            return;
        }

        switch (feedback_code)
        {
        case 1:
            ESP_LOGI(TAG, "seguimiento devuelve un ok");
            jsend(conf->to_screen_queue, ScreenMsg, {
                msg->command = Flash;
                msg->data.icon = OK_Icon;
            });
            break;

        case 2:
            jsend(conf->to_screen_queue, ScreenMsg, {
                msg->command = Flash;
                msg->data.icon = OtherClass_Icon;
            });
            break;
        default:
            // Ha ido bien?
        }
    }
    else if (0 == strcmp(method, "seguimiento_err"))
    {
        // 400 - formato de un dato incorrecto
        // 404 - la id no existe
        // 422 - formato del mensaje incorrecto

        int err_code = 0;

        if (json_obj_get_int(&jctx, "status_code", &err_code) != OS_SUCCESS)
        {
        }

        ESP_LOGI(TAG, "ERR CODE: %d", err_code);

        switch (err_code)
        {
        case 404:

            jsend(conf->to_screen_queue, ScreenMsg, {
                msg->command = Flash;
                msg->data.icon = NotFound_Icon;
            });
            break;

        default:

            // unknown error

            break;
        }
    }
    else if (0 == strcmp(method, "ble"))
    {
        struct ConnectionParameters parameters;
        j_nvs_get(nvs_conf_tag, &parameters, sizeof(struct ConnectionParameters));
        /*
            {
                "macs": [
                    "00:11:22:33:FF:EE",
                    "11:22:33:44:AA:BB"
                ]
            }
        */
        int mac_count = 0;
        json_obj_get_array(&jctx, "macs", &mac_count);

        for (int i = 0; i < mac_count; i++)
        {
            char mac[18];
            char decoded_mac[6];
            json_arr_get_string(&jctx, i, mac, 18);
            sscanf(mac, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", &decoded_mac[0], &decoded_mac[1], &decoded_mac[2], &decoded_mac[3], &decoded_mac[4], &decoded_mac[5]);

            struct bt_device_record device_history[BT_DEVICE_HISTORY_SIZE];
            get_bt_device_history(device_history);

            for (int i = 0; i < BT_DEVICE_HISTORY_SIZE; i++)
            {
                // log the macs we are comparing
                if (VALID_ENTRY(device_history[i]))
                {
                    ESP_LOGI(TAG, "comparing %02X:%02X:%02X:%02X:%02X:%02X with %02X:%02X:%02X:%02X:%02X:%02X", device_history[i].address[0], device_history[i].address[1], device_history[i].address[2], device_history[i].address[3], device_history[i].address[4], device_history[i].address[5], decoded_mac[0], decoded_mac[1], decoded_mac[2], decoded_mac[3], decoded_mac[4], decoded_mac[5]);

                    if (memcmp(device_history[i].address, decoded_mac, 6) == 0)
                    {
                        /*
                           {
                                "tipo_registro": "RegistroSeguimientoDispositivoBle",
                                "espacioId": 1,
                                "dispositivoId": 1,
                                "mac": "00:11:22:33:FF:EE"
                            }
                        */
                        char params[200];
                        snprintf(params, sizeof(params), "{"
                                                         "  \"tipo_registro\": \"RegistroSeguimientoDispositivoBle\","
                                                         "  \"espacioId\": %d,"
                                                         "  \"dispositivoId\": %d,"
                                                         "  \"mac\": \"%s\""
                                                         "}",
                                 parameters.qr_info.space_id, parameters.backend_info.device_id, mac);

                        send_api_post("seguimiento", params);

                        break;
                    }
                }
            }
        }
    }
}
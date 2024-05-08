#pragma once
#include "mqtt.h"
#include "nvs_plugin.h"
#include "esp_crt_bundle.h"

static const char *TAG = "mqtt";

void tb_shared_attribute_ingest(jparse_ctx_t *jctx, struct MQTTConf *conf, bool requested)
{
    char ping_delay_secs_string[20];

    if (json_obj_get_string(jctx, "ping_delay", ping_delay_secs_string, sizeof(ping_delay_secs_string)) == OS_SUCCESS)
    {
        int ping_delay_secs = atoi(ping_delay_secs_string);
        ESP_LOGE(TAG, "updated ping_delay: %d", ping_delay_secs);

        set_ping_delay(ping_delay_secs * 1000 / portTICK_PERIOD_MS);
    }

    char totp_form_base_url[URL_SIZE];

    if (json_obj_get_string(jctx, "totp_form_base_url", totp_form_base_url, sizeof(totp_form_base_url)) == OS_SUCCESS)
    {
        ESP_LOGE(TAG, "updated totp_form_base_url: %s", totp_form_base_url);

        struct ConnectionParameters parameters;
        j_nvs_get(nvs_conf_tag, &parameters, sizeof(struct ConnectionParameters));
        memcpy(parameters.qr_info.totp_form_base_url, totp_form_base_url, URL_SIZE);
        j_nvs_set(nvs_conf_tag, &parameters, sizeof(struct ConnectionParameters));
    }

    char fw_title[30];

    if (json_obj_get_string(jctx, "fw_title", fw_title, sizeof(fw_title)) == OS_SUCCESS)
    {
        char fw_url[URL_SIZE];

        jsend(conf->to_ota_queue, OTAMsg, {
            msg->command = Update;
            msg->requested = requested;
            
            if (json_obj_get_string(jctx, "fw_url", fw_url, URL_SIZE) == OS_SUCCESS)
            {
                snprintf(msg->url, OTA_URL_SIZE, "%s", fw_url);
            }
            else
            {
                char fw_version[30];
                json_obj_get_string(jctx, "fw_version", fw_version, sizeof(fw_version));
                struct ConnectionParameters parameters;
                j_nvs_get(nvs_conf_tag, &parameters, sizeof(struct ConnectionParameters));

                snprintf(msg->url, OTA_URL_SIZE, "%s/api/v1/%s/firmware/?title=%s&version=%s", parameters.qr_info.thingsboard_url, parameters.access_token, fw_title, fw_version);
            }
            ESP_LOGI(TAG, "installing new firmware from: %s", msg->url);
        });
    }
}

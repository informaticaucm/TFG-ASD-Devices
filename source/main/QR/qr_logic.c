#include "qr.h"
#include "../Starter/starter.h"
#include "../MQTT/mqtt.h"
#include "../Camera/camera.h"
#include "../Screen/screen.h"

#include <stdio.h>
#include <sys/param.h>
#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_heap_caps.h"
#include "driver/spi_master.h"
#include "driver/sdmmc_host.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "bsp/esp-bsp.h"
#include "sdkconfig.h"
#include "quirc.h"
#include "quirc_internal.h"
#include "esp_camera.h"
#include "src/misc/lv_color.h"
#include "json_parser.h"
#include "../SYS_MODE/sys_mode.h"

#include "../common.h"
#include "../Camera/camera.h"

#define CONFIG_TRANSIMISSION_TIMEOUT_SEC 20;

void qr_seen(struct QRConf *conf, char *data, int len)
{
    ESP_LOGI(TAG, "the contents were: %s", qr_data.payload);

    if (strncmp("reconf", (char *)qr_data.payload, 6) == 0)
    {
        char *payload_segment_validation = NULL;
        char *payload_buffer = NULL;
        int segment_size = 0;
        int segment_count = 0;
        start_time = 0;

        char packet_type[20];
        jparse_ctx_t jctx;
        json_parse_start(&jctx, (char *)qr_data.payload + 6, qr_data.payload_len - 6);

        json_obj_get_string(&jctx, "packet_type", packet_type, 20);

        if (strncmp(packet_type, "start", 5) == 0)
        {
            start_time = time(0);
            json_obj_get_int(&jctx, "segment_size", &segment_size);
            json_obj_get_int(&jctx, "segment_count", &segment_count);

            payload_segment_validation = jalloc(segment_count * sizeof(char) + 1);
            memset(payload_segment_validation, '_', segment_count * sizeof(char));
            payload_segment_validation[segment_count] = '\0';
            payload_buffer = jalloc(segment_count * segment_size);
        }
        else if (strncmp(packet_type, "segment", 7) == 0)
        {
            if (payload_segment_validation != NULL && payload_buffer != NULL)
            {
                int segment_id;
                json_obj_get_int(&jctx, "segment_id", &segment_id);
                json_obj_get_string(&jctx, "segment", payload_buffer + segment_id * segment_size, segment_size);
                payload_segment_validation[segment_id] = '#';
            }
        }

        bool complete_payload = true;

        for (int i = 0; i < segment_count; i++)
        {
            if (payload_segment_validation[i] == '_')
            {
                complete_payload = false;
                break;
            }
        }

        {
            ScreenMsg *msg = jalloc(sizeof(ScreenMsg));
            msg->command = StateWarning;

            snprintf(msg->data.text, sizeof(msg->data.text), "Recieving configuration\n%s", payload_segment_validation);
            int res = xQueueSend(conf->to_screen_queue, &msg, 0);
            if (res != pdTRUE)
            {
                free(msg);
            }

            set_tmp_mode(state_display, 1, mirror);
        }

        bool end_of_transimision = false;

        if (complete_payload)
        {
            struct StarterMsg *msg = jalloc(sizeof(struct StarterMsg));

            msg->command = QrInfo;

            jparse_ctx_t jctx;
            json_parse_start(&jctx, (char *)payload_buffer, payload_buffer_len);

            json_obj_get_string(&jctx, "device_name", msg->data.qr.qr_info.device_name, 50);
            json_obj_get_int(&jctx, "space_id", &msg->data.qr.qr_info.space_id);
            json_obj_get_string(&jctx, "thingsboard_url", msg->data.qr.qr_info.thingsboard_url, URL_SIZE);
            json_obj_get_string(&jctx, "mqtt_broker_url", msg->data.qr.qr_info.mqtt_broker_url, URL_SIZE);
            json_obj_get_string(&jctx, "provisioning_device_key", msg->data.qr.qr_info.provisioning_device_key, 21);
            json_obj_get_string(&jctx, "provisioning_device_secret", msg->data.qr.qr_info.provisioning_device_secret, 21);
            json_obj_get_string(&jctx, "wifi_psw", msg->data.qr.qr_info.wifi_psw, 30);
            json_obj_get_string(&jctx, "wifi_ssid", msg->data.qr.qr_info.wifi_ssid, 30);

            ESP_LOGI(TAG, "json field device_name %s", msg->data.qr.qr_info.device_name);
            ESP_LOGI(TAG, "json field space_id %d", msg->data.qr.qr_info.space_id);
            ESP_LOGI(TAG, "json field thingsboard_url %s", msg->data.qr.qr_info.thingsboard_url);
            ESP_LOGI(TAG, "json field mqtt_broker_url %s", msg->data.qr.qr_info.mqtt_broker_url);
            ESP_LOGI(TAG, "json field provisioning_device_key %s", msg->data.qr.qr_info.provisioning_device_key);
            ESP_LOGI(TAG, "json field provisioning_device_secret %s", msg->data.qr.qr_info.provisioning_device_secret);
            ESP_LOGI(TAG, "json field wifi_psw %s", msg->data.qr.qr_info.wifi_psw);
            ESP_LOGI(TAG, "json field wifi_ssid %s", msg->data.qr.qr_info.wifi_ssid);

            int res = xQueueSend(conf->to_starter_queue, &msg, 0);
            if (res != pdTRUE)
            {
                free(msg);
            }
            end_of_transimision = true;
        }

        end_of_transimision |= (time(0) - start_time) > CONFIG_TRANSIMISSION_TIMEOUT_SEC;

        if (end_of_transimision)
        {
            {
                ScreenMsg *msg = jalloc(sizeof(ScreenMsg));
                msg->command = StateWarning;

                snprintf(msg->data.text, sizeof(msg->data.text), "Recieving configuration is over", payload_segment_validation);
                int res = xQueueSend(conf->to_screen_queue, &msg, 0);
                if (res != pdTRUE)
                {
                    free(msg);
                }

                set_tmp_mode(state_display, 1, mirror);
            }

            free(payload_buffer);
            payload_buffer = NULL;
            free(payload_segment_validation);
            payload_segment_validation = NULL;
            segment_count = 0;
            segment_size = 0;

            set_mode(mirror);
        }
    }
    else
    {
        struct MQTTMsg *msg = jalloc(sizeof(struct MQTTMsg));

        msg->command = Found_TUI_qr;
        strcpy((char *)msg->data.found_tui_qr.TUI_qr, (char *)qr_data.payload);

        int res = xQueueSend(conf->to_mqtt_queue, &msg, 0);
        if (res != pdTRUE)
        {
            free(msg);
        }
    }
}
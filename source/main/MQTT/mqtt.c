#define mqtt_qos 2

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_system.h"
#include "esp_partition.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"

#include "esp_log.h"
#include "mqtt_client.h"
#include "esp_tls.h"
#include "esp_ota_ops.h"
#include <sys/param.h>
#include "../common.h"

#include "mqtt.h"
#include "../OTA/ota.h"
#include "../Starter/starter.h"
#include "json_parser.h"

static const char *TAG = "mqtt";
extern const uint8_t server_cert_pem_start[] asm("_binary_ca_cert_pem_start");
extern const uint8_t server_cert_pem_end[] asm("_binary_ca_cert_pem_end");

esp_mqtt_client_handle_t client = 0;

// mosquitto_pub -d -q 1 -h thingsboard.asd -p 1883 -t v1/devices/me/telemetry -u c9YTwKgDDaaMMA5oVv6z -m "{temperature:25}"

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

    if (strcmp(topic, "v1/devices/me/attributes") == 0)
    {
        struct OTAMsg *msg = malloc(sizeof(struct OTAMsg));
        int err = json_obj_get_string(&jctx, "fw_url", msg->url, URL_SIZE);

        if (err == OS_SUCCESS)
        {
            ESP_LOGI(TAG, "installing new firmware from: %s", msg->url);

            int res = xQueueSend(conf->to_ota_queue, &msg, 0);
            if (res != pdTRUE)
            {
                free(msg);
            }
        }
        else
        {
            ESP_LOGE(TAG, "ERROR ON JSON KEY EXTRACTION: %d", err);
            free(msg);
        }
    }
    else if (strcmp(topic, "/provision/response") == 0)
    {
        /*{
            "status":"SUCCESS",
            "credentialsType":"ACCESS_TOKEN",
            "credentialsValue":"sLzc0gDAZPkGMzFVTyUY"
        }*/
        struct StarterMsg *msg = malloc(sizeof(struct StarterMsg));
        msg->command = ProvisioningInfo;
        int err = json_obj_get_string(&jctx, "credentialsValue", msg->data.provisioning.access_tocken, 21);

        if (err == OS_SUCCESS)
        {
            ESP_LOGI(TAG, "access tocken is : %s", msg->data.provisioning.access_tocken);

            int res = xQueueSend(conf->to_starter_queue, &msg, 0);
            if (res != pdTRUE)
            {
                free(msg);
            }
        }
        else
        {
            ESP_LOGE(TAG, "ERROR ON JSON KEY EXTRACTION: %d", err);
            free(msg);
        }
    }
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%" PRIi32, base, event_id);
    esp_mqtt_event_handle_t event = event_data;

    struct MQTTConf *conf = handler_args;

    switch ((esp_mqtt_event_id_t)event_id)
    {

    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
        printf("DATA=%.*s\r\n", event->data_len, event->data);

        char *topic_buff = alloca(event->topic_len + 1);
        char *data_buff = alloca(event->data_len + 1);

        memcpy(topic_buff, event->topic, event->topic_len);
        memcpy(data_buff, event->data, event->data_len);
        topic_buff[event->topic_len] = 0;
        data_buff[event->data_len] = 0;

        mqtt_listener(topic_buff, data_buff, conf);

        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT)
        {
            ESP_LOGI(TAG, "Last error code reported from esp-tls: 0x%x", event->error_handle->esp_tls_last_esp_err);
            ESP_LOGI(TAG, "Last tls stack error number: 0x%x", event->error_handle->esp_tls_stack_err);
            ESP_LOGI(TAG, "Last captured errno : %d (%s)", event->error_handle->esp_transport_sock_errno,
                     strerror(event->error_handle->esp_transport_sock_errno));
        }
        else if (event->error_handle->error_type == MQTT_ERROR_TYPE_CONNECTION_REFUSED)
        {
            ESP_LOGI(TAG, "Connection refused error: 0x%x", event->error_handle->connect_return_code);
        }
        else
        {
            ESP_LOGW(TAG, "Unknown error type: 0x%x", event->error_handle->error_type);
        }
        break;
    default:
        ESP_LOGI(TAG, "Other mqtt event id:%d", event->event_id);
        break;
    }
}

void mqtt_subscribe(char *topic)
{
    ESP_LOGI(TAG, "subscribing to: %s", topic);

    esp_mqtt_client_subscribe(client, topic, mqtt_qos);
}

void mqtt_send(char *topic, char *msg)
{
    ESP_LOGI(TAG, "sending: %s to %s", msg, topic);

    esp_mqtt_client_publish(client, topic, msg, strlen(msg), mqtt_qos, 0);
}

void mqtt_send_telemetry(char *msg)
{
    mqtt_send("v1/devices/me/telemetry", msg);
}

void mqtt_send_ota_status_report(enum OTAState status)
{
    // {"current_fw_title": "myFirmware", "current_fw_version": "1.2.3", "fw_state": "UPDATED"}

    char *to_string[] = {
        "DOWNLOADING",
        "DOWNLOADED",
        "VERIFIED",
        "UPDATING",
        "UPDATED",
    };

    char msg[100];
    snprintf(msg, 100, "{\"fw_state\": \"%s\"}", to_string[status]);
    mqtt_send_telemetry(msg);
}

void mqtt_send_ota_fail(char *explanation)
{
    // {"fw_state": "FAILED", "fw_error":  "the human readable message about the cause of the error"}
    char msg[150];
    snprintf(msg, 150, "{\"fw_state\": \"FAILED\", \"fw_error\": \"%s\"}", explanation);
    mqtt_send_telemetry(msg);
}

void mqtt_task(void *arg)
{
    struct MQTTConf *conf = arg;

    while (1)
    {
        // ESP_LOGI(TAG, "tick");

        struct MQTTMsg *msg;
        if (xQueueReceive(conf->to_mqtt_queue, &msg, TASK_DELAY) != pdPASS)
        {
            continue;
        }

        ESP_LOGI(TAG, "a message was recieved at mqtt module");

        switch (msg->command)
        {
        case OTA_failure:
            mqtt_send_ota_fail(msg->data.ota_failure.failure_msg);
            break;
        case OTA_state_update:
            mqtt_send_ota_status_report(msg->data.ota_state_update.ota_state);
            break;
        case Found_TUI_qr:
            char str_buff[MAX_QR_SIZE + 40];

            snprintf(str_buff, MAX_QR_SIZE + 40, "{qr_code: \"%s\"}", msg->data.found_tui_qr.TUI_qr);

            mqtt_send_telemetry(str_buff); // TODO send read qr through mqtt
            break;
        case DoProvisioning:
        {
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
            ESP_LOGI(TAG, "a");
            ESP_ERROR_CHECK(esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, arg));
            ESP_LOGI(TAG, "b");

            ESP_ERROR_CHECK(esp_mqtt_client_start(client));
            ESP_LOGI(TAG, "c");

            mqtt_subscribe("/provision/response");
            ESP_LOGI(TAG, "d");

            /*{
                "deviceName": "DEVICE_NAME",
                "provisionDeviceKey": "PUT_PROVISION_KEY_HERE",
                "provisionDeviceSecret": "PUT_PROVISION_SECRET_HERE"
            }*/

            char msg_buffer[200];
            snprintf(msg_buffer, 200, "{"
                                      "\"deviceName\": \"%s\","
                                      "\"provisionDeviceKey\": \"%s\","
                                      "\"provisionDeviceSecret\": \"%s\""
                                      "}",
                     msg->data.provisioning.device_name,
                     msg->data.provisioning.provisioning_device_key,
                     msg->data.provisioning.provisioning_device_secret);
            ESP_LOGI(TAG, "e");

            mqtt_send("/provision/request", msg_buffer);

            ESP_LOGI(TAG, "f");

            // memcpy(&conf->broker_url, &msg->data.start.broker_url, URL_SIZE);
        }
        break;
        case Start:
        {
            ESP_LOGI(TAG, "start conf");
            ESP_LOGI(TAG, "brocker url %s", msg->data.start.broker_url);
            ESP_LOGI(TAG, "access_tocken %s", msg->data.start.access_tocken);

            const esp_mqtt_client_config_t mqtt_cfg = {
                .broker = {
                    .address.uri = msg->data.start.broker_url,
                    .verification.certificate = (const char *)server_cert_pem_start},
                .credentials = {
                    .username = msg->data.start.access_tocken,
                }};

            client = esp_mqtt_client_init(&mqtt_cfg);
            ESP_ERROR_CHECK(esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, arg));
            ESP_ERROR_CHECK(esp_mqtt_client_start(client));
            mqtt_subscribe("v1/devices/me/attributes");

            if (conf->send_updated_mqtt_on_start)
            {
                mqtt_send_ota_status_report(UPDATED);
            }

            mqtt_send_telemetry("{online:\"true\"}");

            // memcpy(&conf->broker_url, &msg->data.start.broker_url, URL_SIZE);
        }
        break;
        }

        free(msg);
    }
}

void mqtt_start(struct MQTTConf *conf)
{
    TaskHandle_t handle = jTaskCreate(&mqtt_task, "MQTT Task", 30000, conf, 1, MALLOC_CAP_SPIRAM);
    if (handle == NULL)
    {
        ESP_LOGE(TAG, "Problem on task start");
        heap_caps_print_heap_info(MALLOC_CAP_DEFAULT);
    }
}
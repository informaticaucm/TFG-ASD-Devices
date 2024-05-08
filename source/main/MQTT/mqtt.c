#define mqtt_qos 2

#include "mqtt.h"
#include "mqtt_engine.c"
#include <sys/param.h>

#define TAG "mqtt"

void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    // ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%d" PRIi32, base, event_id);
    esp_mqtt_event_handle_t event = event_data;

    struct MQTTConf *conf = handler_args;

    switch ((esp_mqtt_event_id_t)event_id)
    {

    case MQTT_EVENT_SUBSCRIBED:
        // ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        // ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        // ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
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

void send_api_post(char *path, char *request_body)
{
    int len = strlen(request_body) + strlen(path) + 50;
    char *rpc_params = alloca(len);
    snprintf(rpc_params, len, "{\"path\": \"%s\", \"request_body\": %s}", path, request_body);

    mqtt_send_rpc("api_post", rpc_params);
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

int rpc_id = 0;

void mqtt_send_rpc(char *method, char *params)
{
    rpc_id++;
    char topic[50];
    snprintf(topic, sizeof(topic), "v1/devices/me/rpc/request/%d", rpc_id);
    char *msg = alloca(30 + strlen(method) + strlen(params));
    snprintf(msg, 30 + strlen(method) + strlen(params), "{\"method\": \"%s\", \"params\": %s}", method, params);
    mqtt_send(topic, msg);
}

void mqtt_send_ota_status_report(enum OTAState status)
{
    // {"current_fw_title": "myFirmware", "current_fw_version": "1.2.3", "fw_state": "UPDATED"}

    if (status < 0 || status > 4)
    {
        ESP_LOGE(TAG, "invalid status %d", status);
        return;
    }
    char *to_string[] = {
        [DOWNLOADING] = "DOWNLOADING",
        [DOWNLOADED] = "DOWNLOADED",
        [VERIFIED] = "VERIFIED",
        [UPDATING] = "UPDATING",
        [UPDATED] = "UPDATED",
    };

    char msg[100];
    snprintf(msg, sizeof(msg), "{\"fw_state\": \"%s\"}", to_string[status]);
    mqtt_send_telemetry(msg);
}

void mqtt_send_ota_fail(char *explanation)
{
    // {"fw_state": "FAILED", "fw_error":  "the human readable message about the cause of the error"}
    char msg[150];
    snprintf(msg, sizeof(msg), "{\"fw_state\": \"FAILED\", \"fw_error\": \"%s\"}", explanation);
    mqtt_send_telemetry(msg);
}

void mqtt_start(struct MQTTConf *conf)
{
    TaskHandle_t handle = jTaskCreate(&mqtt_task, "MQTT Task", 30000, conf, 1, MALLOC_CAP_SPIRAM);
    if (handle == NULL)
    {
        ESP_LOGE(TAG, "Problem on task start");
        heap_caps_print_heap_info(MALLOC_CAP_SPIRAM);
    }
}

void mqtt_ask_for_atributes()
{
    mqtt_send("v1/devices/me/attributes/request/1",
              "{\"clientKeys\":\"attribute1,attribute2\", \"sharedKeys\":\"fw_checksum,fw_checksum_algorithm,fw_size,fw_tag,fw_title,fw_version,ping_delay\"}");
}
#define broker_url "mqtts://thingsboard.asd:8883"
#define device_id "c9YTwKgDDaaMMA5oVv6z"
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

static const char *TAG = "mtqq_plugin";
extern const uint8_t server_cert_pem_start[] asm("_binary_ca_cert_pem_start");
extern const uint8_t server_cert_pem_end[] asm("_binary_ca_cert_pem_end");

void (*data_callback)(char *, char *);

esp_mqtt_client_handle_t client = 0;

// mosquitto_pub -d -q 1 -h thingsboard.asd -p 1883 -t v1/devices/me/telemetry -u c9YTwKgDDaaMMA5oVv6z -m "{temperature:25}"

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%" PRIi32, base, event_id);
    esp_mqtt_event_handle_t event = event_data;
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

        char* topic_buff = alloca(event->topic_len + 1);
        char *data_buff = alloca(event->data_len + 1);

        memcpy(topic_buff, event->topic, event->topic_len);
        memcpy(data_buff, event->data, event->data_len);
        topic_buff[event->topic_len] = 0;
        data_buff[event->data_len] = 0;

        data_callback(topic_buff, data_buff);

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
    ESP_LOGI(TAG, "subscribing to: %s at %s", topic, broker_url);

    esp_mqtt_client_subscribe(client, topic, mqtt_qos);
}

void mqtt_send(char *topic, char *msg)
{
    ESP_LOGI(TAG, "sending: %s to %s at %s", msg, topic, broker_url);

    esp_mqtt_client_publish(client, topic, msg, strlen(msg), mqtt_qos, 0);
}

void mqtt_init(void (*callback)(char *, char *))
{
    data_callback = callback;
    const esp_mqtt_client_config_t mqtt_cfg = {
        .broker = {
            .address.uri = broker_url,
            .verification.certificate = (const char *)server_cert_pem_start},
        .credentials = {
            .username = device_id,
        }};
    while (client == 0)
    {
        client = esp_mqtt_client_init(&mqtt_cfg);
        ESP_LOGI(TAG, "client: %d", (int)client);
    }

    /* The last argument may be used to pass data to the event handler, in this example mqtt_event_handler */
    ESP_ERROR_CHECK(esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL));
    ESP_ERROR_CHECK(esp_mqtt_client_start(client));
}
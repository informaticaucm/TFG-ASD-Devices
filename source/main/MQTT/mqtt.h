#ifndef __MQTT_H__
#define __MQTT_H__
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
#include "../SYS_MODE/sys_mode.h"

#include "../OTA/ota.h"
#include "../Starter/starter.h"
#include "../Screen/screen.h"
#include "../SYS_MODE/sys_mode.h"
#include "json_parser.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "../common.h"

extern const uint8_t server_cert_pem_start[] asm("_binary_ca_cert_pem_start");
extern const uint8_t server_cert_pem_end[] asm("_binary_ca_cert_pem_end");

enum MQTTCommand
{
    OTA_failure,
    OTA_state_update,
    Found_TUI_qr,
    Start,
    LogInToServer,
    DoProvisioning,
    SendPingToServer,
    FetchBTMacs,
    TagScanned,
};

enum OTAState
{
    DOWNLOADING,
    DOWNLOADED,
    VERIFIED,
    UPDATING,
    UPDATED,
};

struct MQTTMsg
{
    enum MQTTCommand command;
    union
    {
        struct
        {
            char *failure_msg;
        } ota_failure;
        struct
        {
            char TUI_qr[MAX_QR_SIZE];
        } found_tui_qr;
        struct
        {
            char broker_url[URL_SIZE];
            char access_token[21];
        } start;
        struct
        {
            char broker_url[URL_SIZE];
            char device_name[50];
            char provisioning_device_secret[21];
            char provisioning_device_key[21];
        } provisioning;
        struct
        {
            char name[50];
            int space_id;
        } login;
        struct
        {
            int space_id;
        } fetch_btmacs;
        struct
        {
            enum OTAState ota_state;
        } ota_state_update;
        struct 
        {
            uint64_t sn;
        } tag_scanned;
    } data;
};

struct MQTTConf
{
    QueueHandle_t to_mqtt_queue;
    QueueHandle_t to_ota_queue;
    QueueHandle_t to_screen_queue;
    QueueHandle_t to_starter_queue;
    int send_updated_mqtt_on_start;
};

void mqtt_start(struct MQTTConf *conf);
void mqtt_subscribe(char *topic);
void mqtt_send(char *topic, char *msg);
void mqtt_send_telemetry(char *msg);
void mqtt_send_rpc(char *method, char *params);
void send_api_post(char *path, char *request_body);
void mqtt_send_ota_status_report(enum OTAState status);
void mqtt_send_ota_fail(char *explanation);
void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);

#endif
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "../common.h"

enum MQTTCommand
{
    OTA_failure,
    OTA_state_update,
    Found_TUI_qr,
    Start,
    DoProvisioning
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
            char TUI_qr[1024 * 8];
        } found_tui_qr;
        struct
        {
            char broker_url[URL_SIZE];
            char access_tocken[21];
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
            enum OTAState ota_state;
        } ota_state_update;
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
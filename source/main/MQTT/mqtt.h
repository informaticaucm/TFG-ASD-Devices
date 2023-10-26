#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

enum MQTTCommand
{
    OTA_failure,
        OTA_state_update,
    found_TUI_qr,
    start,
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
    MQTTCommand command;
    // OTA_failure
    char *failure_msg;
    // OTA_state_update
    OTA_STATE ota_state;
    // found_TUI_qr
    char *TUI_qr;
    // start
    char *broker_url;
}

struct MQTTConf
{
    QueueHandle_t qr_to_mqtt_queue;
    QueueHandle_t mqtt_to_ota_queue;
    QueueHandle_t ota_to_mqtt_queue;
    QueueHandle_t mqtt_to_screen_queue;
    QueueHandle_t starter_to_mqtt_queue;
};

void mqtt_start(MQTTConf conf);
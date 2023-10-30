#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "../common.h"

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
    enum MQTTCommand command;
    // OTA_failure
    char failure_msg[100];
    // OTA_state_update
    enum OTAState ota_state;
    // found_TUI_qr
    char TUI_qr[URL_SIZE];
    // start
    char broker_url[URL_SIZE];
};

struct MQTTConf
{
    QueueHandle_t qr_to_mqtt_queue;
    QueueHandle_t mqtt_to_ota_queue;
    QueueHandle_t ota_to_mqtt_queue;
    QueueHandle_t mqtt_to_screen_queue;
    QueueHandle_t starter_to_mqtt_queue;
    char broker_url[URL_SIZE];
};

void mqtt_start(struct MQTTConf conf);
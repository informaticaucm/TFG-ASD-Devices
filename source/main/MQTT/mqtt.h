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
    union 
    {
        struct
        {
            char *failure_msg;
        } ota_failure;
        struct
        {
            char TUI_qr[URL_SIZE];
        } found_tui_qr;
        struct
        {
            char broker_url[URL_SIZE];
        } start;
        struct
        {
            enum OTAState ota_state;
        } ota_state_update;
    } data;
};

struct MQTTConf
{
    QueueHandle_t qr_to_mqtt_queue;
    QueueHandle_t mqtt_to_ota_queue;
    QueueHandle_t ota_to_mqtt_queue;
    QueueHandle_t mqtt_to_screen_queue;
    QueueHandle_t starter_to_mqtt_queue;
    int send_updated_mqtt_on_start;
};

void mqtt_start(struct MQTTConf *conf);
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "../common.h"

struct StartMsg
{
    char wifi_ssid[30];
    char wifi_psw[30];
    char mqtt_broker_url[URL_SIZE];
};

struct StarterConf
{
    QueueHandle_t starter_to_screen_queue;
    QueueHandle_t qr_to_starter_queue;
    QueueHandle_t starter_to_mqtt_queue;
};

void start_starter(struct StarterConf *conf);
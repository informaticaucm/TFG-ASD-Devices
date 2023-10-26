#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

struct StartMsg{
    char *wifi_ssid;
    char *wifi_psw;
    char *mqtt_broker;
};

struct StarterConf{
    QueueHandle_t starter_to_screen_queue;
    QueueHandle_t qr_to_starter_queue;
    QueueHandle_t starter_to_mqtt_queue;
};

void start_starter(StarterConf conf);
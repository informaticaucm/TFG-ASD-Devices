#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

enum Command
{
    DrawQr,
    DisplayWarning,
    DisplayInfo,
    DisplayError,
};

struct ScreenMsg
{
    Command comand;
    char *data;
};

struct ScreenConf{
    QueueHandle_t starter_to_screen_queue;
    QueueHandle_t mqtt_to_screen_queue;
    QueueHandle_t ota_to_sceen_queue;
};
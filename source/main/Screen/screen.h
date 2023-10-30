#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

enum ScreenCommand
{
    DrawQr,
    DisplayWarning,
    DisplayInfo,
    DisplayError,
};

struct ScreenMsg
{
    enum ScreenCommand comand;
    char data[100];
};

struct ScreenConf{
    QueueHandle_t starter_to_screen_queue;
    QueueHandle_t mqtt_to_screen_queue;
    QueueHandle_t ota_to_screen_queue;
};

void screen_start(struct ScreenConf *conf);
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

enum ScreenCommand
{
    DrawQr,
    DisplayWarning,
    DisplayInfo,
    DisplayError,
    DisplayProgress,
    DisplayProcessing,
};

struct ScreenMsg
{
    enum ScreenCommand command;
    union
    {
        char text[100];
        struct
        {
            char text[90];
            float progress;
        } progress;
    } data;
};

struct ScreenConf
{
    QueueHandle_t starter_to_screen_queue;
    QueueHandle_t mqtt_to_screen_queue;
    QueueHandle_t ota_to_screen_queue;
};

void screen_start(struct ScreenConf *conf);
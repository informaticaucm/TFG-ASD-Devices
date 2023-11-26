#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

enum ScreenCommand
{
    Empty,
    DrawQr,
    DisplayWarning,
    DisplayInfo,
    DisplayError,
    DisplayProgress,
    DisplayProcessing,
    DisplayImage,
};

struct ScreenMsg
{
    enum ScreenCommand command;
    union
    {
        char text[MAX_QR_SIZE];
        struct
        {
            char text[90];
            float progress;
        } progress;
        struct
        {
            uint8_t *buf;
            int width;
            int height;
        } image;
    } data;
};

struct ScreenConf
{
    QueueHandle_t to_screen_queue;
};

void screen_start(struct ScreenConf *conf);
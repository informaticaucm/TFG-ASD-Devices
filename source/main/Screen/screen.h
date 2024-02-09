#ifndef __SCREEN_H__
#define __SCREEN_H__
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "../common.h"
#include "../Camera/camera.h"

enum ScreenCommand
{
    Empty,
    DrawQr,
    DisplayWarning,
    DisplaySuccess,
    DisplayError,
    DisplayText,
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
        struct meta_frame *mf;
    } data;
};

struct ScreenConf
{
    QueueHandle_t to_screen_queue;
};

void screen_start(struct ScreenConf *conf);

#endif
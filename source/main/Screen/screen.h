#ifndef __SCREEN_H__
#define __SCREEN_H__
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "../common.h"
#include "../SYS_MODE/sys_mode.h"
#include "../Camera/camera.h"
#include "../common.h"
#include "../BT/bt.h"

enum ScreenCommand
{
    DrawQr,

    StateWarning,
    StateSuccess,
    StateError,
    StateText,

    Mirror,
};

struct ScreenMsg
{
    enum ScreenCommand command;
    union
    {
        char text[MAX_QR_SIZE];
        struct meta_frame *mf;
    } data;
};

struct ScreenConf
{
    QueueHandle_t to_screen_queue;
};

void screen_start(struct ScreenConf *conf);

#endif
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
    Flash,

    DrawQr,

    StarterStateInform,

    Mirror,

    ShowMsg,
};

enum Icon{
    OK_Icon,
    NotFound_Icon,
    OtherClass_Icon,
};

struct ScreenMsg
{
    enum ScreenCommand command;
    union
    {
        enum StarterState starter_state;
        char text[MAX_QR_SIZE];
        struct meta_frame *mf;
        enum Icon icon;
    } data;
};

struct ScreenConf
{
    QueueHandle_t to_screen_queue;
};

void screen_start(struct ScreenConf *conf);

#endif
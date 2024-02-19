#ifndef __SCREEN_H__
#define __SCREEN_H__
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "../common.h"
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

    BTUpdate,

    Mirror,
};

struct ScreenMsg
{
    enum ScreenCommand command;
    union
    {
        bt_device_record_t bt_devices[BT_DEVICE_HISTORY_SIZE];
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
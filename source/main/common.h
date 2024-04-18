#pragma once

#include "common.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/portmacro.h"
#include "esp_heap_caps.h"

// #define IMG_WIDTH 640
// #define IMG_HEIGHT 480
// #define CAM_FRAME_SIZE FRAMESIZE_VGA // 640x480

// #define IMG_WIDTH 1024
// #define IMG_HEIGHT 768
// #define CAM_FRAME_SIZE FRAMESIZE_XGA // 1024x768

// #define IMG_WIDTH 800
// #define IMG_HEIGHT 600
// #define CAM_FRAME_SIZE FRAMESIZE_SVGA

#define IMG_WIDTH 240
#define IMG_HEIGHT 240
#define CAM_FRAME_SIZE FRAMESIZE_240X240

// #define IMG_WIDTH 400
// #define IMG_HEIGHT 296
// #define CAM_FRAME_SIZE FRAMESIZE_CIF // 400x296

#define nvs_conf_tag "ConnParams"

#define PING_RATE 60

#define DEFAULT_TASK_DELAY 50
#define DEFAULT_RT_TASK_DELAY 1
#define DEFAULT_IDLE_TASK_DELAY 500
#define DEFAULT_PING_DELAY 90000 / portTICK_PERIOD_MS

#define MAX_QR_SIZE 300
#define URL_SIZE 100
#define OTA_URL_SIZE 256

#define BT_DEVICE_HISTORY_SIZE 10
#define BT_DEVICE_HISTORY_MAX_AGE 60 * 60  // 1h

#define min(a,b) (((a) < (b)) ? (a) : (b))
#define max(a,b) (((a) > (b)) ? (a) : (b))

#define jTaskCreate xTaskCreateCap
#define jalloc(x) heap_caps_malloc(x, MALLOC_CAP_SPIRAM);

TaskHandle_t xTaskCreateCap(TaskFunction_t pxTaskCode,
                            const char *const pcName,
                            const uint32_t ulStackDepth,
                            void *const pvParameters,
                            UBaseType_t uxPriority, uint32_t caps); // creates a task using psram instead of internal

#define jsend(queue, msgType, msgPrep) \
    {                                   \
        struct msgType *msg = jalloc(sizeof(struct msgType)); \
        {msgPrep}                        \
        int res = xQueueSend(queue, &msg, 0);     \
        if (res != pdTRUE)              \
        {                               \
            free(msg);                  \
        }                               \
    }

#define jsend_with_free(queue, msgType, msgPrep, freeProcedure) { \
        struct msgType *msg = jalloc(sizeof(struct msgType)); \
        {msgPrep}                        \
        int res = xQueueSend(queue, &msg, 0);     \
        if (res != pdTRUE)              \
        {                               \
            {freeProcedure}                    \
            free(msg);                  \
        }                               \
    }
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

#define IMG_WIDTH 240
#define IMG_HEIGHT 240
#define CAM_FRAME_SIZE FRAMESIZE_240X240 // 240x240


// #define IMG_WIDTH 400
// #define IMG_HEIGHT 296
// #define CAM_FRAME_SIZE FRAMESIZE_CIF // 400x296

#define nvs_conf_tag "conf"

#define PING_RATE 10
#define PING_TIMEOUT 3

#define MAX_QR_SIZE 300
#define URL_SIZE 100
#define TASK_DELAY 50
#define RT_TASK_DELAY 30

#define min(a,b) (((a) < (b)) ? (a) : (b))

#define jTaskCreate xTaskCreateCap
#define jalloc(x) heap_caps_malloc(x, MALLOC_CAP_SPIRAM);

TaskHandle_t xTaskCreateCap(TaskFunction_t pxTaskCode,
                            const char *const pcName,
                            const uint32_t ulStackDepth,
                            void *const pvParameters,
                            UBaseType_t uxPriority, uint32_t caps); // creates a task using psram instead of internal

#pragma once

#include "common.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/portmacro.h"
#include "esp_heap_caps.h"

#define IMG_WIDTH 240
#define IMG_HEIGHT 240
#define CAM_FRAME_SIZE FRAMESIZE_240X240
#define URL_SIZE 100
#define TASK_DELAY 50
#define RT_TASK_DELAY 30

#define jTaskCreate xTaskCreateCap
#define jalloc malloc

TaskHandle_t xTaskCreateCap(TaskFunction_t pxTaskCode,
                           const char *const pcName,
                           const uint32_t ulStackDepth,
                           void *const pvParameters,
                           UBaseType_t uxPriority, uint32_t caps); // creates a task using psram instead of internal

#include "common.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/portmacro.h"
#include "esp_heap_caps.h"
#include "esp_log.h"

TaskHandle_t xTaskCreateCap(TaskFunction_t pxTaskCode,
                            const char *const pcName,
                            const uint32_t ulStackDepth,
                            void *const pvParameters,
                            UBaseType_t uxPriority, uint32_t caps)
{ // creates a task using psram instead of internal ram

    StackType_t *const puxStackBuffer = heap_caps_malloc(ulStackDepth * sizeof(StackType_t), caps);
    StaticTask_t *const pxTaskBuffer = heap_caps_malloc(sizeof(StaticTask_t), MALLOC_CAP_INTERNAL); // tiene que ser interna y byte addreseable
    xPortCheckValidTCBMem(pxTaskBuffer);
    return xTaskCreateStatic(pxTaskCode, pcName, ulStackDepth, pvParameters, uxPriority, puxStackBuffer, pxTaskBuffer);
}
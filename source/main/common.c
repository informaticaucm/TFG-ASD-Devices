#include "common.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/portmacro.h"
#include "esp_heap_caps.h"
#include "esp_log.h"

TaskHandle_t xTaskCreatePS(TaskFunction_t pxTaskCode,
                           const char *const pcName,
                           const uint32_t ulStackDepth,
                           void *const pvParameters,
                           UBaseType_t uxPriority)
{ // creates a task using psram instead of internal

    ESP_LOGI("xTaskCreatePS", "single largest posible allocation: %d", heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM));
    ESP_LOGI("xTaskCreatePS", "required alocations %d", (int)(ulStackDepth * sizeof(StackType_t)));

    StackType_t *const puxStackBuffer = heap_caps_malloc(ulStackDepth * sizeof(StackType_t), MALLOC_CAP_SPIRAM);

    ESP_LOGI("xTaskCreatePS", "single largest posible allocation: %d", heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL));
    ESP_LOGI("xTaskCreatePS", "required alocations %d", (int)(sizeof(StaticTask_t)));
    StaticTask_t *const pxTaskBuffer = heap_caps_malloc(sizeof(StaticTask_t), MALLOC_CAP_INTERNAL); // tiene que ser interna y byte accesible
    xPortCheckValidTCBMem(pxTaskBuffer);
    return xTaskCreateStatic(pxTaskCode, pcName, ulStackDepth, pvParameters, uxPriority, puxStackBuffer, pxTaskBuffer);
}
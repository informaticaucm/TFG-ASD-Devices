#include "sys_mode.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

struct sys_mode_state state;
SemaphoreHandle_t xSemaphore;
bool started = false;


void init()
{
    if (!started)
    {
        xSemaphore = xSemaphoreCreateMutex();
        started = true;
    }
}

void set_mode(enum sys_mode mode)
{
    init();
    if (xSemaphoreTake(xSemaphore, portMAX_DELAY))
    {
        state.mode = mode;

        xSemaphoreGive(xSemaphore);
    }
}

enum sys_mode get_mode()
{
    init();
    enum sys_mode ret = DEFAULT_SYS_MODE;
    if (xSemaphoreTake(xSemaphore, portMAX_DELAY))
    {
        ret = state.mode;

        xSemaphoreGive(xSemaphore);
    }

    return ret;
}

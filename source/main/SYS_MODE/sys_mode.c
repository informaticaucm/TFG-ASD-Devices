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

void get_parameters(struct ConfigurationParameters *parameters)
{
    init();
    if (xSemaphoreTake(xSemaphore, portMAX_DELAY))
    {
        memcpy(parameters, &state.parameters, sizeof(struct ConfigurationParameters));

        xSemaphoreGive(xSemaphore);
    }
}

void set_parameters(struct ConfigurationParameters *parameters)
{
    init();
    if (xSemaphoreTake(xSemaphore, portMAX_DELAY))
    {
        memcpy(&state.parameters, parameters, sizeof(struct ConfigurationParameters));

        xSemaphoreGive(xSemaphore);
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

set_tmp_mode(enum sys_mode mode, int sec_duration, enum sys_mode next_mode)
{
    init();
    if (xSemaphoreTake(xSemaphore, portMAX_DELAY))
    {
        state.tmp_mode = mode;
        state.tmp_mode_expiration = time(0) + sec_duration;
        state.mode = next_mode;

        xSemaphoreGive(xSemaphore);
    }
}

enum sys_mode get_mode()
{
    init();
    enum sys_mode ret = mirror;
    if (xSemaphoreTake(xSemaphore, portMAX_DELAY))
    {
        if (state.tmp_mode_expiration > time(0))
        {
            ret = state.tmp_mode;
        }
        else
        {
            ret = state.mode;
        }
        xSemaphoreGive(xSemaphore);
    }

    return ret;
}

#include "sys_mode.h"
#include "esp_log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "../common.h"

#define TAG "sys_mode"

struct sys_mode_state state = {
    .rt_task_delay = DEFAULT_RT_TASK_DELAY,
    .task_delay = DEFAULT_TASK_DELAY,
    .ota_running = false,
};
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

void set_version(char version[32])
{
    init();
    if (xSemaphoreTake(xSemaphore, portMAX_DELAY))
    {
        strcpy(state.version, version);

        xSemaphoreGive(xSemaphore);
    }
}

void get_version(char version[32])
{
    init();
    if (xSemaphoreTake(xSemaphore, portMAX_DELAY))
    {
        strcpy(version, state.version);

        xSemaphoreGive(xSemaphore);
    }
}

void set_tmp_mode(enum sys_mode mode, int sec_duration, enum sys_mode next_mode)
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

void set_rt_task_delay(int rt_task_delay)
{
    init();
    if (xSemaphoreTake(xSemaphore, portMAX_DELAY))
    {
        state.rt_task_delay = rt_task_delay;

        xSemaphoreGive(xSemaphore);
    }
}
int get_rt_task_delay()
{
    init();
    int ret = 0;
    if (xSemaphoreTake(xSemaphore, portMAX_DELAY))
    {
        ret = state.rt_task_delay;

        xSemaphoreGive(xSemaphore);
    }

    return ret;
}
void set_task_delay(int task_delay)
{
    init();
    if (xSemaphoreTake(xSemaphore, portMAX_DELAY))
    {
        state.task_delay = task_delay;

        xSemaphoreGive(xSemaphore);
    }
}
int get_task_delay()
{
    init();
    int ret = 0;
    if (xSemaphoreTake(xSemaphore, portMAX_DELAY))
    {
        ret = state.task_delay;

        xSemaphoreGive(xSemaphore);
    }

    return ret;
}
void set_ota_running(bool ota_running)
{
    init();
    if (xSemaphoreTake(xSemaphore, portMAX_DELAY))
    {
        state.ota_running = ota_running;
      
        xSemaphoreGive(xSemaphore);
    }

}
bool is_ota_running()
{
    init();
    bool ret = false;
    if (xSemaphoreTake(xSemaphore, portMAX_DELAY))
    {
        ret = state.ota_running;

        xSemaphoreGive(xSemaphore);
    }

    return ret;
}
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
    .totp_ready = false,
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

#define critical_section(section)                  \
    init();                                        \
    if (xSemaphoreTake(xSemaphore, portMAX_DELAY)) \
    {                                              \
        {section} xSemaphoreGive(xSemaphore);      \
    }

void set_mode(enum sys_mode mode)
{
    critical_section(state.mode = mode;)
}

void set_tmp_mode(enum sys_mode mode, int sec_duration, enum sys_mode next_mode)
{
    critical_section(state.tmp_mode = mode;
                     state.tmp_mode_expiration = time(0) + sec_duration;
                     state.mode = next_mode;)
}

enum sys_mode get_mode()
{
    enum sys_mode ret = mirror;
    critical_section(
        if (state.tmp_mode_expiration > time(0)) {
            ret = state.tmp_mode;
        } else {
            ret = state.mode;
        }) return ret;
}

void get_parameters(struct ConfigurationParameters *parameters)
{
    critical_section(memcpy(parameters, &state.parameters, sizeof(struct ConfigurationParameters));)
}

void set_parameters(struct ConfigurationParameters *parameters)
{
    critical_section(memcpy(&state.parameters, parameters, sizeof(struct ConfigurationParameters));)
}

void set_version(char version[32])
{
    critical_section(strcpy(state.version, version);)
}

void get_version(char version[32])
{
    critical_section(strcpy(version, state.version);)
}

void set_rt_task_delay(int rt_task_delay)
{
    critical_section(state.rt_task_delay = rt_task_delay;)
}

int get_rt_task_delay()
{
    int ret = 0;
    critical_section(ret = state.rt_task_delay;) return ret;
}

void set_task_delay(int task_delay)
{
    critical_section(state.task_delay = task_delay;)
}

int get_task_delay()
{
    int ret = 0;
    critical_section(ret = state.task_delay;) return ret;
}

void set_ota_running(bool ota_running)
{
    critical_section(state.ota_running = ota_running;)
}

bool is_ota_running()
{
    bool ret = false;
    critical_section(ret = state.ota_running;) return ret;
}

void set_qr_url_template(char qr_url_template[URL_SIZE])
{
    critical_section(strcpy(state.qr_url_template, qr_url_template);)
}

void get_qr_url_template(char qr_url_template[URL_SIZE])
{
    critical_section(strcpy(qr_url_template, state.qr_url_template);)
}

void set_TOTP_secret(char TOTP_secret[17])
{
    critical_section(strcpy(state.TOTP_secret, TOTP_secret);)
}

void get_TOTP_secret(char TOTP_secret[17])
{
    critical_section(strcpy(TOTP_secret, state.TOTP_secret);)
}

void set_TOTP_t0(int t0)
{
    critical_section(state.TOTP_t0 = t0;)
}

int get_TOTP_t0()
{
    int ret = 0;
    critical_section(ret = state.TOTP_t0;) return ret;
}

void set_TOTP_ready(bool totp_ready)
{
    critical_section(state.totp_ready = totp_ready;)
}

bool is_totp_ready()
{
    bool ret = false;
    critical_section(ret = state.totp_ready;) return ret;
}

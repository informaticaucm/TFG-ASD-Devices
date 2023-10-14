#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

SemaphoreHandle_t swriters;
SemaphoreHandle_t sreaders;

int num_readers = 0;
int num_writers = 0;

#define wait_time 100000

void init_sync_system()
{
    swriters = xSemaphoreCreateCounting(1, 0);
    sreaders = xSemaphoreCreateCounting(1, 0);
}

void reader_start()
{
    xSemaphoreTake(sreaders, wait_time);
    if (++num_readers == 1)
        xSemaphoreTake(swriters, wait_time);
    xSemaphoreGive(sreaders);
}

void reader_end()
{
    xSemaphoreTake(sreaders, wait_time);
    if (--num_readers == 0)
        xSemaphoreGive(swriters);
    xSemaphoreGive(sreaders);
}

void writer_start()
{
    xSemaphoreTake(swriters, wait_time);
}

void writer_end()
{
    xSemaphoreGive(swriters);
}
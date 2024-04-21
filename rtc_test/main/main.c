#include "freertos/FreeRTOS.h"
#include <esp_log.h>
#include <time.h>
#include "freertos/task.h"

void app_main(void)
{
    while (1)
    {
        vTaskDelay(1000);
        int t = time(NULL);
        printf(" %010d \n", t);
    }
}

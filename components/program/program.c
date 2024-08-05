#include <stdio.h>
#include "program.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"

static const char *TAG = "secondary";

void log_actions()
{
    //ESP_LOGI(TAG, "hello");
    ESP_LOGI(TAG, "free memory: %i", (int)esp_get_free_heap_size());
}

void program()
{
    while(true)
    {
        log_actions();
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}

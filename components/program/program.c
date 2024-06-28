#include <stdio.h>
#include "program.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"

static const char *TAG = "secondary";

void log_actions()
{
    ESP_LOGI(TAG, "hello");
    ESP_LOGI(TAG, "free memory: %i", heap_caps_get_minimum_free_size(MALLOC_CAP_8BIT));
}

void program()
{
    while(true)
    {
        log_actions();
        vTaskDelay(4000 / portTICK_PERIOD_MS);
    }
}

#include <stdio.h>
#include "program.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"

#include "server_comm.h"
#include "etatherm.h"

static const char *TAG = "secondary";

static void defaultCallback(char* str){
    ESP_LOGI(TAG ,"callback with arg: %s", str);
}

void log_actions()
{
    //ESP_LOGI(TAG, "hello");
    ESP_LOGI(TAG, "free memory: %i", (int)esp_get_free_heap_size());
}

void program()
{
    etatherm_err_t result;
    uint8_t value = 0;
    commAddAction("test-program", defaultCallback);
    result = etatherm_init();
    ESP_LOGI(TAG, "uart init: %d", result);
    while(true)
    {
        log_actions();
        result = etathermGetRealTemp(0, &value);
        ESP_LOGI(TAG, "error result: %d value: %u", result, value);
        vTaskDelay(20000 / portTICK_PERIOD_MS);
    }
}

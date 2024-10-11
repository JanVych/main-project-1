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
    // eta_err_t result;
    // uint8_t value = 0;
    // commAddAction("test-program", defaultCallback);
    // result = etaInit();
    // ESP_LOGI(TAG, "uart init: %d", result);
    // uint8_t room = 0;
    // uint8_t setval = 13;
    while(true)
    {
        log_actions();
        // result = etaGetRealTemp(room, &value);
        // ESP_LOGI(TAG, "real temp result: %d, room: %u, value: %u", result, room, value);
        // result = etaGetDesiredTemp(room, &value);
        // ESP_LOGI(TAG, "desired temp result: %d, room: %u, value: %u", result, room, value);
        // result = etaGetOzTemp(room, &value);
        // ESP_LOGI(TAG, "oz temp result: %d, room: %u, value: %u", result, room, value);

        // result = etaSetOzTemp(0, setval);
        // ESP_LOGI(TAG, "set oz temp result: %d, room: %u, value: %u", result, room, setval);

        // vTaskDelay(20000 / portTICK_PERIOD_MS);
    }
}

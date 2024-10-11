#include <stdio.h>
#include "program.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "driver/uart.h"

#include "server_comm.h"
#include "etatherm.h"

static const char *TAG = "secondary";

static void defaultCallback(char* str){
    ESP_LOGI(TAG ,"callback with arg: %s", str);
}

void log_actions()
{
    ESP_LOGI(TAG, "free memory: %i", (int)esp_get_free_heap_size());
}

void program()
{
    // eta_err_t result;
    // uint8_t value = 0;
    // commAddAction("test-program", defaultCallback);
    // result = eta_Init(UART_NUM_2, 17, 16);
    // ESP_LOGI(TAG, "uart init: %d", result);
    // uint8_t room = 2;
    // uint8_t setval = 14;
    while(true)
    {
        log_actions();
        // result = eta_GetRealTemp(1, room, &value);
        // ESP_LOGI(TAG, "real temp result: %d, room: %u, value: %u", result, room, value);
        // result = eta_GetDesiredTemp(1, room, &value);
        // ESP_LOGI(TAG, "desired temp result: %d, room: %u, value: %u", result, room, value);
        // result = eta_GetOzTemp(1, room, &value);
        // ESP_LOGI(TAG, "oz temp result: %d, room: %u, value: %u", result, room, value);

        // result = eta_SetOzTemp(1,room, setval);
        // ESP_LOGI(TAG, "set oz temp result: %d, room: %u, value: %u", result, room, setval);

        // result = eta_GetTempShift(1, 0, &value);
        // ESP_LOGI(TAG, "type value: %d, room: %u, value: %u", result, room, value);


        vTaskDelay(20000 / portTICK_PERIOD_MS);
    }
}

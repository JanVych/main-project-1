#include <stdio.h>
#include <cJSON.h>
#include "program.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "driver/uart.h"

#include "wattrouter_mx.h"

#include "server_comm.h"

static const char *TAG = "program";

static int64_t _data[5] = {0, 0, 0, 0, 0};

int32_t _counter = 0;

int32_t ProgramCounter()
{
    return _counter;
}

int32_t FeedingPower()
{
    return (int32_t)_data[0];
}

int32_t BoilerEnergy()
{
    return (int32_t)_data[1];
}

int32_t BoilerPower()
{
    return (int32_t)_data[2];
}

int32_t TuvEnergy()
{
    return (int32_t)_data[3];
}

int32_t TuvPower()
{
    return (int32_t)_data[4];
}

void OnProgramDestroy()
{
}

void Main()
{
    comm_AddMessageI32("ProgramCounter", ProgramCounter);
    // comm_AddMessageI32("FeedingPower", FeedingPower);
    comm_AddMessageI32("BoilerEnergy", BoilerEnergy);
    comm_AddMessageI32("BoilerPower", BoilerPower);
    comm_AddMessageI32("TuvEnergy", TuvEnergy);
    comm_AddMessageI32("TuvPower", TuvPower);

    wattrouter_Init();

    vTaskDelay(6000 / portTICK_PERIOD_MS);

    while (true)
    {
        // wattrouter_GetFeedingPower(&_data[0]);
        // ESP_LOGI("UART", "Feeding power: %ld", _data[0]);

        vTaskDelay(1000 / portTICK_PERIOD_MS);
        wattrouter_GetBoilerEnergy(&_data[1]);
        ESP_LOGI("UART", "Boiler energy: %lld", _data[1]);

        vTaskDelay(1000 / portTICK_PERIOD_MS);
        wattrouter_GetBoilerPower(&_data[2]);
        ESP_LOGI("UART", "Boiler power: %lld", _data[2]);

        vTaskDelay(1000 / portTICK_PERIOD_MS);
        wattrouter_GetTuvEnergy(&_data[3]);
        ESP_LOGI("UART", "Tuv energy: %lld", _data[3]);

        vTaskDelay(1000 / portTICK_PERIOD_MS);
        wattrouter_GetTuvPower(&_data[4]);
        ESP_LOGI("UART", "Tuv power: %lld", _data[4]);

        vTaskDelay(20000 / portTICK_PERIOD_MS);
        _counter++;
    }
}

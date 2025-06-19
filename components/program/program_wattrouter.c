// #include <stdio.h>
// #include <cJSON.h>
// #include "program.h"
// #include "esp_log.h"
// #include "freertos/FreeRTOS.h"

// #include "wattrouter_mx.h"

// #include "server_comm.h"

// //static const char *TAG = "program";

// static int64_t _data[5] = {0, 0, 0, 0};
// static int32_t _feedingPower = 0;

// static bool _feedingPowerSetMode= false;
// static int32_t _feedingPowerSet = 0;

// int32_t _counter = 0;

// int32_t ProgramCounter()
// {
//     return _counter;
// }

// int32_t FeedingPower()
// {
//     return _feedingPower;
// }

// int32_t BoilerEnergy()
// {
//     return (int32_t)_data[0];
// }

// int32_t BoilerPower()
// {
//     return (int32_t)_data[1];
// }

// int32_t TuvEnergy()
// {
//     return (int32_t)_data[2];
// }

// int32_t TuvPower()
// {
//     return (int32_t)_data[3];
// }

// void SetFeedingPower(int32_t value)
// {
//     _feedingPowerSet = value;
//     _feedingPowerSetMode = true;
// }

// void SetFeedingPowerSetMode(bool mode)
// {
//     _feedingPowerSetMode = mode;
// }

// void OnProgramDestroy()
// {
// }

// void Main()
// {
//     comm_AddMessageI32("ProgramCounter", ProgramCounter);
//     comm_AddMessageI32("FeedingPower", FeedingPower);
//     comm_AddMessageI32("BoilerEnergy", BoilerEnergy);
//     comm_AddMessageI32("BoilerPower", BoilerPower);
//     comm_AddMessageI32("TuvEnergy", TuvEnergy);
//     comm_AddMessageI32("TuvPower", TuvPower);

//     comm_AddActionInt32("SetFeedingPower", SetFeedingPower);
//     comm_AddActionBool("SetFeedingPowerSetMode", SetFeedingPowerSetMode);

//     wattrouter_Init();

//     vTaskDelay(6000 / portTICK_PERIOD_MS);
//     esp_err_t result;

//     while (true)
//     {
//         // wattrouter_SetTuvState(WATT_ROUTER_STATE_AUTO);
//         // wattrouter_SetAccuState(WATT_ROUTER_STATE_AUTO);
//         // wattrouter_SetFeedingPower(200);

//         if(_feedingPowerSetMode)
//         {
//             ESP_LOGI("UART", "Set feeding power: %ld", _feedingPowerSet);
//             result = wattrouter_SetFeedingPower(_feedingPowerSet);
//             ESP_ERROR_CHECK_WITHOUT_ABORT(result);
//             if(result == ESP_OK)
//             {
//                 ESP_LOGI("UART", "Feeding power set: %ld", _feedingPowerSet);
//             }
//         }
//         vTaskDelay(1000 / portTICK_PERIOD_MS);

//         ESP_LOGI("UART", "Get feeding power");
//         result = wattrouter_GetFeedingPower(&_feedingPower);
//         ESP_ERROR_CHECK_WITHOUT_ABORT(result);
//         if(result == ESP_OK)
//         {
//             ESP_LOGI("UART", "Feeding power: %ld", _feedingPower);
//         }
//         vTaskDelay(100 / portTICK_PERIOD_MS);

//         ESP_LOGI("UART", "Get accu energy");
//         result = wattrouter_GetAccuEnergy(&_data[0]);
//         ESP_ERROR_CHECK_WITHOUT_ABORT(result);
//         if(result == ESP_OK)
//         {
//             ESP_LOGI("UART", "Accu energy: %lld", _data[0]);
//         }
//         vTaskDelay(100 / portTICK_PERIOD_MS);

//         ESP_LOGI("UART", "Get accu power");
//         result = wattrouter_GetAccuPower(&_data[1]);
//         ESP_ERROR_CHECK_WITHOUT_ABORT(result);
//         if(result == ESP_OK)
//         {
//             ESP_LOGI("UART", "Accu power: %lld", _data[1]);
//         }
//         vTaskDelay(100 / portTICK_PERIOD_MS);

//         ESP_LOGI("UART", "Get Tuv energy");
//         result = wattrouter_GetTuvEnergy(&_data[2]);
//         ESP_ERROR_CHECK_WITHOUT_ABORT(result);
//         if(result == ESP_OK)
//         {
//             ESP_LOGI("UART", "Tuv energy: %lld", _data[2]);
//         }
//         vTaskDelay(100 / portTICK_PERIOD_MS);

//         ESP_LOGI("UART", "Get Tuv power");
//         result = wattrouter_GetTuvPower(&_data[3]);
//         ESP_ERROR_CHECK_WITHOUT_ABORT(result);
//         if(result == ESP_OK)
//         {
//             ESP_LOGI("UART", "Tuv power: %lld", _data[3]);
//         }

//         vTaskDelay(10000 / portTICK_PERIOD_MS);
//         _counter++;
//     }
// }

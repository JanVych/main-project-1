// #include <stdio.h>
// #include <cJSON.h>
// #include "program.h"
// #include "esp_log.h"
// #include "freertos/FreeRTOS.h"
// #include "driver/uart.h"

// #include "wattrouter_mx.h"
// #include "server_comm.h"
// #include "ecomax.h"


// static int64_t _wattrouterData[5] = {0, 0, 0, 0};
// static int32_t _feedingPower = 0;

// static bool _feedingPowerSetMode= false;
// static int32_t _feedingPowerSet = 0;

// static ecomax_data_t _ecomaxData = {0};

// static int32_t _counter = 0;

// int32_t ProgramCounter(){
//     return _counter;
// }
// int32_t FeedingPower(){
//     return _feedingPower;
// }
// int32_t BoilerEnergy(){
//     return _wattrouterData[0];
// }
// int32_t BoilerPower(){
//     return _wattrouterData[1];
// }
// int32_t TuvEnergy(){
//     return _wattrouterData[2];
// }
// int32_t TuvPower(){
//     return _wattrouterData[3];
// }

// void SetFeedingPower(int32_t value)
// {
//     _feedingPowerSet = value;
//     _feedingPowerSetMode = true;
// }
// void SetFeedingPowerMode(bool mode)
// {
//     _feedingPowerSetMode = mode;
// }

// float MixTemperature(){
//     return _ecomaxData.mixTemperature;
// }
// float FlueTemperature(){
//     return _ecomaxData.flueTemperature;
// }
// float TuvTemperature(){
//     return _ecomaxData.tuvTemperature;
// }
// float BoilerTemperature(){
//     return _ecomaxData.boilerTemperature;
// }
// float AcuUpperTemperature(){
//     return _ecomaxData.acuUpperTemperature;
// }
// float AcuBottomTemperature(){
//     return _ecomaxData.acuBottomTemperature;
// }
// float OutsideTemperature(){
//     return _ecomaxData.outsideTemperature;
// }
// float OxygenLevel(){
//     return _ecomaxData.oxygenLevel;
// }

// void OnProgramDestroy()
// {
// }

// void Main()
// {
//     //watt router
//     comm_AddMessageI32("ProgramCounter", ProgramCounter);
//     comm_AddMessageI32("FeedingPower", FeedingPower);
//     comm_AddMessageI32("BoilerEnergy", BoilerEnergy);
//     comm_AddMessageI32("BoilerPower", BoilerPower);
//     comm_AddMessageI32("TuvEnergy", TuvEnergy);
//     comm_AddMessageI32("TuvPower", TuvPower);

//     comm_AddActionInt32("SetFeedingPower", SetFeedingPower);
//     comm_AddActionBool("SetFeedingPowerMode", SetFeedingPowerMode);

//     wattrouter_Init(UART_NUM_2, 25, 26);

//     //ecomax
//     ecomax_Init(UART_NUM_0, 17, 16);
//     comm_AddMessageFloat("MixTemperature", MixTemperature);
//     comm_AddMessageFloat("FlueTemperature", FlueTemperature);
//     comm_AddMessageFloat("TuvTemperature", TuvTemperature);
//     comm_AddMessageFloat("BoilerTemperature", BoilerTemperature);
//     comm_AddMessageFloat("AcuUpperTemperature", AcuUpperTemperature);
//     comm_AddMessageFloat("AcuBottomTemperature", AcuBottomTemperature);
//     comm_AddMessageFloat("OutsideTemperature", OutsideTemperature);
//     comm_AddMessageFloat("OxygenLevel", OxygenLevel);

//     vTaskDelay(6000 / portTICK_PERIOD_MS);

//     while (true)
//     {
//         ESP_LOGI("PROGRAM","Program start");
//         // wattrouter_SetTuvState(WATT_ROUTER_STATE_AUTO);
//         // wattrouter_SetAccuState(WATT_ROUTER_STATE_AUTO);

//         if(_feedingPowerSetMode)
//         {
//             ESP_ERROR_CHECK_WITHOUT_ABORT(wattrouter_SetFeedingPower(_feedingPowerSet));
//         }
//         vTaskDelay(1000 / portTICK_PERIOD_MS);

//         ESP_ERROR_CHECK_WITHOUT_ABORT(wattrouter_GetFeedingPower(&_feedingPower));
//         vTaskDelay(100 / portTICK_PERIOD_MS);

//         ESP_ERROR_CHECK_WITHOUT_ABORT(wattrouter_GetAccuEnergy(&_wattrouterData[0]));
//         vTaskDelay(100 / portTICK_PERIOD_MS);

//         ESP_ERROR_CHECK_WITHOUT_ABORT(wattrouter_GetAccuPower(&_wattrouterData[1]));
//         vTaskDelay(100 / portTICK_PERIOD_MS);

//         ESP_ERROR_CHECK_WITHOUT_ABORT(wattrouter_GetTuvEnergy(&_wattrouterData[2]));
//         vTaskDelay(100 / portTICK_PERIOD_MS);

//         ESP_ERROR_CHECK_WITHOUT_ABORT(wattrouter_GetTuvPower(&_wattrouterData[3]));

//         ESP_ERROR_CHECK_WITHOUT_ABORT(ecomax_GetData(&_ecomaxData));

//         _counter++;
//         ESP_LOGI("PROGRAM", "Program end");
//         vTaskDelay(10000 / portTICK_PERIOD_MS);
//     }
// }

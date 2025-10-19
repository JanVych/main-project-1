#include <stdio.h>
#include <cJSON.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "esp_timer.h"
#include "driver/uart.h"

#include "program.h"
#include "server_comm.h"
#include "etatherm.h"

#define ROOM_LABEL_MAX_LEN 32

static const char *TAG = "program";
static int64_t _programStartTime = 0;

static bool _isEnabled = true;

static uint8_t _roomsCurrentTemp[16];
static uint8_t _roomsDesiredTemp[16];
static char _rommsLabel[16][ROOM_LABEL_MAX_LEN] = {
    "Pavel", "Honza", "Room 1", "Living Room",
    "Bath Room", "Hallway", "WC", "",
    "", "", "", "",
    "", "", "", ""
};
static cJSON* BuildJsonArrayFromU8(const uint8_t* data, int count)
{
    cJSON* array = cJSON_CreateArray();
    if (!array) return NULL;

    for (int i = 0; i < count; ++i)
    {
        cJSON* item = cJSON_CreateNumber((double)data[i]);
        if (item)
        {
            cJSON_AddItemToArray(array, item);
        }
    }
    return array;
}

static cJSON* BuildJsonArrayFromStr(const char labels[][ROOM_LABEL_MAX_LEN], int count)
{
    cJSON* array = cJSON_CreateArray();
    if (!array) return NULL;

    for (int i = 0; i < count; ++i)
    {
        cJSON* item = cJSON_CreateString(labels[i]);
        if (item)
        {
            cJSON_AddItemToArray(array, item);
        }
    }
    return array;
}

cJSON* GetRoomsCurrentTemp()
{
    return BuildJsonArrayFromU8(_roomsCurrentTemp, 16);
}

cJSON* GetRoomsDesiredTemp()
{
    return BuildJsonArrayFromU8(_roomsDesiredTemp, 16);
}   

cJSON* GetRoomsLabel()
{
    return BuildJsonArrayFromStr(_rommsLabel, 16);
}

int32_t GetProgramMinutesCounter()
{
    if (_programStartTime == 0) return 0;
    int64_t elapsed_us = esp_timer_get_time() - _programStartTime;
    return (int32_t)(elapsed_us / 60000000LL);
}

void OnProgramDestroy()
{
    eta_Deinit(UART_NUM_2);
}

void LoadDesiredTemps()
{
    eta_err_t result;
    uint8_t value = 0;
    for(int i = 0; i < 16; i++)
    {
        result = eta_GetOzTemp(1, i, &value);
        if(!result){
            ESP_LOGI(TAG, "desired temp result: %d, room: %d, value: %u", result, i, value);
            _roomsDesiredTemp[i] = value;
        }
        else{
            ESP_LOGI(TAG, "desired temp error: %d, room: %d", result, i);
        }
    }
}

void SetEnabled(bool isEnabled)
{
    _isEnabled = isEnabled;
}

void SetDesiredTemps(cJSON* jsonArray)
{
    if (!cJSON_IsArray(jsonArray)) return;

    int arraySize = cJSON_GetArraySize(jsonArray);
    for (int i = 0; i < arraySize && i < 16; ++i)
    {
        cJSON* item = cJSON_GetArrayItem(jsonArray, i);
        if (cJSON_IsNumber(item))
        {
            int value = item->valueint;
            if (value >= 0 && value <= 255)
            {
                _roomsDesiredTemp[i] = (uint8_t)value;
            }
        }
    }
}

void Main()
{
    eta_err_t result;
    uint8_t value = 0;
    _programStartTime = esp_timer_get_time();

    vTaskDelay(5000 / portTICK_PERIOD_MS);
    result = eta_Init(UART_NUM_2, 17, 16);
    ESP_LOGI(TAG, "uart init: %d", result);
    vTaskDelay(10000 / portTICK_PERIOD_MS);
    LoadDesiredTemps(); 

    comm_AddMessageI32("ProgramMinutesCounter", GetProgramMinutesCounter);
    comm_AddMessageJson("RoomsCurrentTemp", GetRoomsCurrentTemp);
    comm_AddMessageJson("RoomsDesiredTemp", GetRoomsDesiredTemp);
    comm_AddMessageJson("RoomsLabel", GetRoomsLabel);

    comm_AddActionJson("SetRoomsDesiredTemp", SetDesiredTemps);
    comm_AddActionBool("SetEnabled", SetEnabled);


    while(true)
    {
        if(_isEnabled)
        {
            ESP_LOGI(TAG, "rooms scan started");

            for(int i = 0; i < 16; i++)
            {
                result = eta_GetRealTemp(1, i, &value);
                
                if(!result){
                    ESP_LOGI(TAG, "real temp result: %d, room: %d, value: %u", result, i, value);
                    _roomsCurrentTemp[i] = value;
                }
                else{
                    ESP_LOGI(TAG, "real temp error: %d, room: %d", result, i);
                }

                result = eta_SetOzTemp(1, i, _roomsDesiredTemp[i]);
                if(!result){
                    ESP_LOGI(TAG, "set oz temp result: %d, room: %d, value: %u", result, i, _roomsDesiredTemp[i]);
                }
                else{
                    ESP_LOGI(TAG, "set oz temp error: %d, room: %d", result, i);
                }

            }
        ESP_LOGI(TAG, "rooms scan ended");
        }
        
        vTaskDelay(60000 / portTICK_PERIOD_MS);
    }
}
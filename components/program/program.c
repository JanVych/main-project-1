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

typedef struct{
    char name[ROOM_LABEL_MAX_LEN];
    uint8_t deviceAddress;
    uint8_t currentTemp;
    uint8_t desiredTemp;
} room_t;

static const char *TAG = "program";
static int64_t _programStartTime = 0;

static bool _isWritingEnabled = true;
static bool _isReadingEnabled = true;

static room_t _rooms[13] =
{
    {"Pavel",0,18,18},
    {"Honza",1,18,18},
    {"Room",2,18,18},
    {"Living room",3,18,18},
    {"Bathroom",4,18,18},
    {"Hallway",5,18,18},
    {"WC",6,18,18},
    {"Room",8,18,18},
    {"Kitchen",9,18,18},
    {"Room",10,18,18},
    {"Hallway",13,18,18},
    {"FLoor",14,18,18},
    {"Boiler",15,40,40}
};

static const size_t _roomCount = sizeof(_rooms) / sizeof(_rooms[0]);

static cJSON* BuildJsonFromRooms(int8_t roomProperthy)
{
    cJSON* array = cJSON_CreateArray();
    if (!array) return NULL;

    for (int i = 0; i < _roomCount; ++i)
    {
        cJSON* item = NULL;
        switch (roomProperthy)
        {
            case 0:
                item = cJSON_CreateString(_rooms[i].name);
                break;
            case 1:
                item = cJSON_CreateNumber(_rooms[i].deviceAddress);
                break;
            case 2:
                item = cJSON_CreateNumber(_rooms[i].currentTemp);
                break;
            case 3:
                item = cJSON_CreateNumber(_rooms[i].desiredTemp);
                break;
            default:
                break;
        }
        if (item)
        {
            cJSON_AddItemToArray(array, item);
        }
    }
    return array;
}

cJSON* GetRoomsCurrentTemp()
{
    return BuildJsonFromRooms(2);
}

cJSON* GetRoomsDesiredTemp()
{
    return BuildJsonFromRooms(3);
}   

cJSON* GetRoomsLabel()
{
    return BuildJsonFromRooms(0);
}

bool GetIsWritingEnabled()
{
    return _isWritingEnabled;
}

bool GetIsReadingEnabled()
{
    return _isReadingEnabled;
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

    for (int i = 0; i < _roomCount; i++)
    {
        result = eta_GetOzTemp(1, _rooms[i].deviceAddress, &_rooms[i].desiredTemp);
        if(!result){
            ESP_LOGI(TAG, "get desired temp, room: %s, value: %u", _rooms[i].name, _rooms[i].desiredTemp);
        }
        else{
            ESP_LOGI(TAG, "desired temp error: %d, room: %s, address: %u", result, _rooms[i].name, _rooms[i].deviceAddress);
        }
    }
}

void SetWritingEnabled(bool isEnabled)
{
    _isWritingEnabled = isEnabled;
}

void SetReadingEnabled(bool isEnabled)
{
    _isReadingEnabled = isEnabled;
}

static void SetDesiredTemps(cJSON* jsonArray)
{
    cJSON* item;
    if (!cJSON_IsArray(jsonArray)) return;
    int arraySize = cJSON_GetArraySize(jsonArray);
    
    for(int i = 0; i < arraySize && i < _roomCount; ++i)
    {
        item = cJSON_GetArrayItem(jsonArray, i);
        if (cJSON_IsNumber(item))
        {
            int value = item->valueint;
            if (value >= 0 && value <= 255)
            {
                _rooms[i].desiredTemp = (uint8_t)value;
            }
        }
    }
}

void Main()
{
    eta_err_t result;
    _programStartTime = esp_timer_get_time();

    vTaskDelay(5000 / portTICK_PERIOD_MS);
    result = eta_Init(UART_NUM_2, 17, 16);
    ESP_LOGI(TAG, "uart init: %d", result);
    vTaskDelay(5000 / portTICK_PERIOD_MS);
    LoadDesiredTemps(); 
    vTaskDelay(5000 / portTICK_PERIOD_MS);

    comm_AddMessageI32("ProgramMinutesCounter", GetProgramMinutesCounter);
    comm_AddMessageJson("RoomsCurrentTemp", GetRoomsCurrentTemp);
    comm_AddMessageJson("RoomsDesiredTemp", GetRoomsDesiredTemp);
    comm_AddMessageJson("RoomsLabel", GetRoomsLabel);
    comm_AddMessageBool("IsWritingEnabled", GetIsWritingEnabled);
    comm_AddMessageBool("IsReadingEnabled", GetIsReadingEnabled);

    comm_AddActionJson("SetRoomsDesiredTemp", SetDesiredTemps);

    comm_AddActionBool("SetReadingEnabled", SetReadingEnabled);
    comm_AddActionBool("SetWritingEnabled", SetWritingEnabled);

    while(true)
    {
        if(_isReadingEnabled || _isWritingEnabled)
        {
            ESP_LOGI(TAG, "rooms scan started");

            for(int i = 0; i < _roomCount; i++)
            {
                if(_isReadingEnabled)
                {
                    result = eta_GetRealTemp(1, _rooms[i].deviceAddress, &_rooms[i].currentTemp);
                    if(!result){
                        ESP_LOGI(TAG, "real temp: room: %s, value: %u", _rooms[i].name, _rooms[i].currentTemp);
                    }
                    else{
                        ESP_LOGI(TAG, "real temp error: %d, room: %s", result, _rooms[i].name);
                    }
                }

                if(_isWritingEnabled)
                {
                    result = eta_SetOzTemp(1, _rooms[i].deviceAddress, _rooms[i].desiredTemp);
                    if(!result){
                        ESP_LOGI(TAG, "set oz temp: room: %s, value: %u", _rooms[i].name, _rooms[i].desiredTemp);
                    }
                    else{
                        ESP_LOGI(TAG, "set oz temp error: %d, room: %s", result, _rooms[i].name);
                    }
                }
            }
            ESP_LOGI(TAG, "rooms scan ended");
        }
        vTaskDelay(60000 / portTICK_PERIOD_MS);
    }
}
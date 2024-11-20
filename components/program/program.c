#include <stdio.h>
#include <cJSON.h>
#include "program.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "driver/uart.h"

#include "server_comm.h"
#include "etatherm.h"

static const char *TAG = "program";

static uint8_t _roomsTemp[16];

int32_t _counter = 0;

int32_t ProgramCounter()
{
    return _counter;
}

void LogActions()
{
    ESP_LOGI(TAG, "free memory: %i", (int)esp_get_free_heap_size());
}

cJSON* GetRoomsTemp()
{
    cJSON* array = cJSON_CreateArray();
    cJSON* item;
    for(int i = 0; i < 16; i++)
    {
        item = cJSON_CreateNumber((double) _roomsTemp[i]);
        cJSON_AddItemToArray(array, item);
    }
    return array;
}

// void SetRoomTemp()
// {

// }

void PrintString(char* str)
{
    ESP_LOGI(TAG, "STR: %s", str);
}

void PrintInt(int32_t value)
{
    ESP_LOGI(TAG, "INT: %li", value);
}

void PrintBool(bool value)
{
    ESP_LOGI(TAG, "BOOL: %i", value);
}

void PrintJson(cJSON* json)
{
    char* str = cJSON_Print(json);
    ESP_LOGI(TAG, "JSON: %s", str);
    free(str);
}

char* SendString()
{
    return "Hello from program";
}

int32_t SendInt()
{
    return 123;
}

bool SendBool()
{
    return true;
}

cJSON* SendJson()
{
    cJSON* array = cJSON_CreateArray();
    cJSON* item;
    for(int i = 0; i < 16; i++)
    {
        item = cJSON_CreateNumber(i);
        cJSON_AddItemToArray(array, item);
    }
    return array;
}

void OnProgramDestroy()
{

}

void Main()
{
    eta_err_t result;
    //uint8_t value = 0;

    comm_AddActionStr("print_string", PrintString);
    comm_AddActionInt32("print_int", PrintInt);
    comm_AddActionBool("print_bool", PrintBool);
    comm_AddActionJson("print_json", PrintJson);

    comm_AddMessageStr("send_string", SendString);
    comm_AddMessageI32("send_int", SendInt);
    comm_AddMessageBool("send_bool", SendBool);
    comm_AddMessageJson("send_json", SendJson);

    comm_AddMessageI32("ProgramCounter", ProgramCounter);
    // comm_AddMessageCjson("RoomsTemperature", GetRoomsTemp);
    vTaskDelay(15000 / portTICK_PERIOD_MS);


    result = eta_Init(UART_NUM_2, 17, 16);
    ESP_LOGI(TAG, "uart init: %d", result);
    // uint8_t room = 2;
    // uint8_t setval = 14;
    while(true)
    {
        // LogActions();
        // comm_PushMessage("program_interval_message", _randomString);
        // _someNumber++;

        // ESP_LOGI(TAG, "rooms scan started");

        // for(int i = 0; i < 16; i++)
        // {
        //     result = eta_GetRealTemp(1, i, &value);
        //     ESP_LOGI(TAG, "real temp result: %d, room: %d, value: %u", result, i, value);
        //     if(!result){
        //         _roomsTemp[i] = value;
        //     }
        // }
        // ESP_LOGI(TAG, "rooms scan ended");


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

        _counter++;
        vTaskDelay(100000 / portTICK_PERIOD_MS);
    }
}

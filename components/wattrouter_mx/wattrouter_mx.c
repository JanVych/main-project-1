#include <stdio.h>
#include <string.h>

#include "esp_timer.h"
#include "driver/uart.h"

#include "esp_log.h"
#include "rs485_modbus_rtu.c"
#include "wattrouter_mx.h"

void wattrouter_Init()
{
    InitPort(UART_NUM_2, 25, 26);
}

// 16bit
esp_err_t wattrouter_GetFeedingPower(int32_t* outValue)
{
    *outValue = 0;
    uint16_t buffer[1];
    ESP_RETURN_ERROR(ReadHoldingRegisters(1, 32, buffer, 1));
    *outValue = - (int16_t) buffer[0] * 10;
    return ESP_OK;
}

esp_err_t wattrouter_SetFeedingPower(int32_t value)
{
    return WriteSingleRegister(1, 32, (uint16_t) (- value / 10));
}

static esp_err_t _SetMultiple(uint16_t address, uint16_t size, uint16_t value)
{
    uint16_t* buffer = malloc(size * sizeof(uint16_t));
    for (int i = 0; i < size; i++)
    {
        buffer[i] = value;
    }
    ESP_RETURN_ERROR(WriteMultipleRegisters(1, address, buffer, size));
    free(buffer);
    return ESP_OK;
}

esp_err_t wattrouter_SetTuvState(watt_router_mx_state_t state)
{
    if(state == WATT_ROUTER_STATE_OFF)
    {
        ESP_RETURN_ERROR(_SetMultiple(0, 3, 0));
        ESP_RETURN_ERROR(_SetMultiple(8, 3, 1000));
    }
    else if(state == WATT_ROUTER_STATE_ON)
    {
        ESP_RETURN_ERROR(_SetMultiple(0, 3, 1000));
        ESP_RETURN_ERROR(_SetMultiple(8, 3, 0));
    }
    else if(state == WATT_ROUTER_STATE_AUTO)
    {
        ESP_RETURN_ERROR(_SetMultiple(0, 3, 0));
        ESP_RETURN_ERROR(_SetMultiple(8, 3, 0));
    }
    return ESP_OK;
}

esp_err_t wattrouter_SetBoilerState(watt_router_mx_state_t state)
{
    if(state == WATT_ROUTER_STATE_OFF)
    {
        ESP_RETURN_ERROR(_SetMultiple(3, 3, 0));
        ESP_RETURN_ERROR(_SetMultiple(11, 3, 1000));
    }
    else if(state == WATT_ROUTER_STATE_ON)
    {
        ESP_RETURN_ERROR(_SetMultiple(3, 3, 1000));
        ESP_RETURN_ERROR(_SetMultiple(11, 3, 0));
    }
    else if(state == WATT_ROUTER_STATE_AUTO)
    {
        ESP_RETURN_ERROR(_SetMultiple(3, 3, 0));
        ESP_RETURN_ERROR(_SetMultiple(11, 3, 0));
    }
    return ESP_OK;
}

esp_err_t wattrouter_GetMultiple(int64_t* outValue, uint16_t* addressArray, uint16_t size)
{
    *outValue = 0;
    uint16_t buffer[2];
    int32_t value;
    for (int i = 0; i < size; i++)
    {
        ESP_RETURN_ERROR(ReadInputRegisters(1, addressArray[i], buffer, 2));
        value = (((int32_t)buffer[0] << 16) | buffer[1]) * 10;
        // ESP_LOGI("UART", "Value %ld", value);
        *outValue += value;
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
    return ESP_OK;
}

esp_err_t wattrouter_GetTuvPower(int64_t* outValue)
{
    uint16_t addressArray[3] = {28, 32, 36};
    return wattrouter_GetMultiple(outValue, addressArray, 3);
}

esp_err_t wattrouter_GetTuvEnergy(int64_t* outValue)
{
    uint16_t addressArray[3] = {30, 34, 38};
    return wattrouter_GetMultiple(outValue, addressArray, 3);
}

esp_err_t wattrouter_GetAccuPower(int64_t* outValue)
{
    uint16_t addressArray[3] = {40, 44, 48};
    return wattrouter_GetMultiple(outValue, addressArray, 3);
}

esp_err_t wattrouter_GetAccuEnergy(int64_t* outValue)
{
    uint16_t addressArray[3] = {42, 46, 50};
    return wattrouter_GetMultiple(outValue, addressArray, 3);
    return 0;
}
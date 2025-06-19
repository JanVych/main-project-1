#include <stdio.h>
#include <string.h>

#include "driver/uart.h"
#include "esp_log.h"
#include "esp_timer.h"

#include "helper.h"
#include "ecomax.h"

#define ECOMAX_READ_TIMEOUT_MS 4000

#define ECOMAX_FRAME_START_DELIMITER 0x68
#define ECOMAX_FRAME_END_DELIMITER 0x16
#define ECOMAX_FRAME_SENDER_TYPE 0xFC

static const char *TAG = "ECO_MAX";

static uart_port_t _port = UART_NUM_0;

esp_err_t ecomax_Init(uart_port_t uartNum, int16_t tx, int32_t rx)
{
    uart_config_t uartConfig = 
    {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };

    ESP_RETURN_ERROR(uart_param_config(uartNum, &uartConfig));
    ESP_RETURN_ERROR(uart_set_pin(uartNum, tx, rx, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    ESP_RETURN_ERROR(uart_driver_install(uartNum, 1024, 1024, 0, NULL, 0));
    _port = uartNum;
    return ESP_OK;
}

esp_err_t ecomax_Deinit(uart_port_t uartNum)
{
    ESP_RETURN_ERROR(uart_driver_delete(uartNum));
    return ESP_OK;
}

static uint8_t _CalculateCrc(uint8_t *data, uint16_t bufferSize) 
{
    uint8_t crc = 0;
    for (int i = 0; i < bufferSize; i++) {
        crc ^= data[i];
    }
    return crc;
}

static esp_err_t _FindFrame(uint8_t* buffer, uint16_t bufferSize) 
{
    uint16_t frameSize;
    uint8_t crc;
    int64_t startTime = esp_timer_get_time();
    int64_t timeoutMs = ECOMAX_READ_TIMEOUT_MS * 1000;

    if (bufferSize < 10)
    {
        return ESP_ERR_INVALID_SIZE;
    }

    while (true)
    {
        int64_t now = esp_timer_get_time();
        if ((now - startTime) > timeoutMs) 
        {
            return ESP_ERR_TIMEOUT;
        }

        uart_read_bytes(_port, buffer, 1, 500 / portTICK_PERIOD_MS);

        if(buffer[0] == ECOMAX_FRAME_START_DELIMITER)
        {
            uart_read_bytes(_port, buffer + 1, 6, 500 / portTICK_PERIOD_MS);
            if (buffer[5] == ECOMAX_FRAME_SENDER_TYPE)
            {
                frameSize = buffer[1] | (buffer[2] << 8);
                if (frameSize > bufferSize)
                {
                    return ESP_ERR_INVALID_SIZE;
                }
                bufferSize = frameSize;
                uart_read_bytes(_port, buffer + 7, frameSize - 7, 500 / portTICK_PERIOD_MS);
                if(buffer[frameSize - 1] != ECOMAX_FRAME_END_DELIMITER)
                {
                    continue;
                }
                crc = _CalculateCrc(buffer, frameSize - 2);
                if (crc != buffer[frameSize - 2])
                {
                    return ESP_ERR_INVALID_CRC;
                }
                return ESP_OK;
            }
        }
    }
    return ESP_ERR_NOT_FOUND;
}

esp_err_t ecomax_PrintFrame()
{
    uint16_t bufferSize = 1024;
    uint8_t buffer[bufferSize];
    ESP_RETURN_ERROR(_FindFrame(buffer, bufferSize));

    ESP_LOGI(TAG, "Recipient address: %02X", buffer[3]);
    ESP_LOGI(TAG, "Sender address: %02X", buffer[4]);
    ESP_LOGI(TAG, "Sender type: %02X", buffer[5]);
    ESP_LOGI(TAG, "ecoNET version: %02X", buffer[6]);

    ESP_LOGI(TAG, "Frame type: %02X", buffer[7]);
    ESP_LOGI(TAG, "Data:");

    for (int i = 8; i < bufferSize - 2; i++)
    {
        printf("%02X ", buffer[i]);
    }
    printf("\n");
    return ESP_OK;
}

static float _GetFloat(const uint8_t *buffer, size_t bufferSize, size_t offset) 
{
    float value = 0.0f;
    if (offset + sizeof(float) <= bufferSize) 
    {
        memcpy(&value, buffer + offset, sizeof(float));
    }
    return value;
}

static int16_t _GetInt16(const uint8_t *buffer, size_t bufferSize, size_t offset) 
{
    int16_t value = 0;
    if (offset + sizeof(int16_t) <= bufferSize)
     {
        memcpy(&value, buffer + offset, sizeof(int16_t));
    }
    return value;
}

esp_err_t ecomax_GetData(ecomax_data_t* data)
{
    uint16_t bufferSize = 1024;
    uint8_t buffer[bufferSize];
    uint8_t* bufferData = buffer + 8;
    uint16_t bufferDataSize = bufferSize - 8;
    int64_t startTime = esp_timer_get_time();
    int64_t timeoutMs = ECOMAX_READ_TIMEOUT_MS * 1000;
    while(true)
    {
        int64_t now = esp_timer_get_time();
        if ((now - startTime) > timeoutMs) 
        {
            return ESP_ERR_TIMEOUT;
        }
        ESP_RETURN_ERROR(_FindFrame(buffer, bufferSize));
        if (buffer[7] == 0x08)
        {
            break;
        }
    }

    data->mixTemperature = _GetFloat(bufferData, bufferDataSize, 78);
    data->flueTemperature = _GetFloat(bufferData, bufferDataSize, 94);
    data->tuvTemperature = _GetFloat(bufferData, bufferDataSize, 98);
    data->boilerTemperature = _GetFloat(bufferData, bufferDataSize, 102);
    data->acuUpperTemperature = _GetFloat(bufferData, bufferDataSize, 110);
    data->acuBottomTemperature = _GetFloat(bufferData, bufferDataSize, 114);
    data->outsideTemperature = _GetFloat(bufferData, bufferDataSize, 86);
    data->oxygenLevel = _GetFloat(bufferData, bufferDataSize, 248);

    // data->fanPower = _GetInt16(buffer, bufferDataSize, 257);

    return ESP_OK;
}

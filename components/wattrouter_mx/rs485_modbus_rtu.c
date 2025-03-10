#include <stdio.h>
#include <string.h>

#include "esp_timer.h"
#include "driver/uart.h"

#include "esp_log.h"

#define ESP_RETURN_ERROR(x) do {    \
    esp_err_t __err_rc = (x);       \
    if (__err_rc != ESP_OK) {       \
        return __err_rc;            \
    }                               \
} while(0)

static uint16_t _Crc16(uint8_t *data, uint16_t size) 
{
    uint16_t crc = 0xFFFF;
    for (uint16_t i = 0; i < size; i++) 
    {
        crc ^= data[i]; 
        for (uint8_t j = 0; j < 8; j++) 
        { 
            if (crc & 0x0001) 
            {
                crc = (crc >> 1) ^ 0xA001;
            } 
            else 
            {
                crc = crc >> 1;
            }
        }
    }
    return crc;
}

static esp_err_t _SendFrame(uint8_t device, uint8_t function, uint16_t address, uint8_t* data, uint16_t size)
{
    uint16_t writtenBytes = 0;
    uint16_t frameSize = 6 + size;
    uint8_t frame[frameSize];
    frame[0] = device;
    frame[1] = function;
    frame[2] = address >> 8;
    frame[3] = address & 0xFF;

    memcpy(frame + 4, data, size);
    uint16_t crc = _Crc16(frame, frameSize - 2);

    frame[4 + size] = crc & 0xFF;
    frame[5 + size] = crc >> 8;

    // ESP_LOGI("UART", "Send frame"); 
    // for (int i = 0; i < frameSize; i++) 
    // {
    //     ESP_LOGI("UART", "Write byte %d: 0x%02X", i, frame[i]);
    // }
    // ESP_LOGI("UART", "---------------------------------"); 
    writtenBytes = uart_write_bytes(UART_NUM_2, frame, frameSize);
    if (writtenBytes != frameSize) 
    {
        return ESP_ERR_INVALID_STATE;
    }
    return ESP_OK;
}

esp_err_t ReadHoldingRegisters(uint8_t device, uint16_t address, uint16_t* outBuffer, uint16_t bufferSize)
{
    uint32_t response_size = 5 + bufferSize * 2;
    uint8_t response[response_size];
    uint8_t data[2];
    data[0] = bufferSize >> 8;
    data[1] = bufferSize & 0xFF;
    ESP_RETURN_ERROR(_SendFrame(device, 3, address, data, 2));
    
    uint16_t len = uart_read_bytes(UART_NUM_2, response, response_size, 1000 / portTICK_PERIOD_MS);
    if (len > 0)
    {
        // ESP_LOGI("UART", "Read %d", len);
        // for (int i = 0; i < len; i++) 
        // {
        //     ESP_LOGI("UART", "Byte %d: 0x%02X", i, response[i]);
        // }
    }
    if(len == response_size)
    {
        for (int i = 0; i < bufferSize; i++)
        {
            outBuffer[i] = response[3 + i * 2] << 8 | response[4 + i * 2];
        }
        return ESP_OK;
    }
    return ESP_ERR_TIMEOUT;
}

esp_err_t WriteSingleRegister(uint8_t device, uint16_t address, uint16_t value)
{
    uint8_t response[8];
    uint8_t data[2];
    data[0] = value >> 8;
    data[1] = value & 0xFF;
    _SendFrame(device, 6, address, data, 2);
    
    int len = uart_read_bytes(UART_NUM_2, response, 8, 1000 / portTICK_PERIOD_MS);
    if (len > 0)
    {
        // ESP_LOGI("UART", "Read %d", len);
        // for (int i = 0; i < len; i++) 
        // {
        //     ESP_LOGI("UART", "Byte %d: 0x%02X", i, response[i]);
        // }
    }
    return 0;
}

esp_err_t WriteMultipleRegisters(uint8_t device, uint16_t address, uint16_t* data, uint16_t size)
{   
    uint8_t buffer[size * 2 + 3];
    uint16_t j = 0;
    uint8_t response[8];

    buffer[0] = size >> 8;
    buffer[1] = size & 0xFF;
    buffer[2] = size * 2;
    
    for (int i = size - 1; i >= 0; i--)
    {
        buffer[3 + j * 2] = data[i] >> 8;
        buffer[4 + j * 2] = data[i] & 0xFF;
        j++;
    }

    ESP_RETURN_ERROR(_SendFrame(device, 16, address, buffer, size * 2 + 3));

    int len = uart_read_bytes(UART_NUM_2, response, 8, 1000 / portTICK_PERIOD_MS);
    if (len > 0)
    {
        // ESP_LOGI("UART", "Read %d", len);
        // for (int i = 0; i < len; i++) 
        // {
        //     ESP_LOGI("UART", "Byte %d: 0x%02X", i, response[i]);
        // }
    }
    return 0;
}

int32_t ReadInputRegisters(uint8_t device, uint16_t address, uint16_t* outBuffer, uint16_t bufferSize)
{
    uint32_t response_size = 5 + bufferSize * 2;
    uint8_t response[response_size];
    uint8_t data[2];
    data[0] = bufferSize >> 8;
    data[1] = bufferSize & 0xFF;
    _SendFrame(device, 4, address, data, 2);
    // if(result != 8)
    // {
    //     return result;
    // }
    
    int len = uart_read_bytes(UART_NUM_2, response, response_size, 1000 / portTICK_PERIOD_MS);
    if (len > 0)
    {
        // ESP_LOGI("UART", "Read %d", len);
        // for (int i = 0; i < len; i++) 
        // {
        //     ESP_LOGI("UART", "Byte %d: 0x%02X", i, response[i]);
        // }
    }
    if(len == response_size)
    {
        for (int i = 0; i < bufferSize; i++)
        {
            outBuffer[i] = response[3 + i * 2] << 8 | response[4 + i * 2];
        }
    }
    return 0;
}

esp_err_t InitPort(uart_port_t uart_num, int16_t tx, int32_t rx)
{
    uart_config_t uart_config = 
    {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };

    ESP_RETURN_ERROR(uart_param_config(uart_num, &uart_config));
    ESP_RETURN_ERROR(uart_set_pin(uart_num, tx, rx, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    ESP_RETURN_ERROR(uart_driver_install(uart_num, 1024, 1024, 0, NULL, 0));
    return ESP_OK;
}

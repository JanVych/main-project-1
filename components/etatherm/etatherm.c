#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "esp_timer.h"
#include "driver/uart.h"
#include "esp_mac.h"
#include "esp_log.h"


#include "etatherm.h"
		
#define	DLE	0x10
#define	SOH	0x01
#define	ETB 0x17

#define	ETA_RETRY 1

#define ETA_HEADER_TIMEOUT_MIL 4000
#define ETA_FRAME_TIMEOUT_MIL 4000
#define ETA_BYTE_TIMEOUT_MIL 200

#define BYTE_TIMEOUT 6//(96 / 16)  	//96 msec  
#define FRAME_RESPONSE 2//(4 / 2)		//4 msec  

static uart_port_t eta_uart_port;

//private
static etatherm_err_t _readData(uint16_t station_a, uint16_t ram_a, uint8_t* out_data, uint8_t data_size);
static etatherm_err_t _readFrame(uint8_t data_size, uint8_t* out_data);
static etatherm_err_t _sendFrame(uint16_t station_a, uint16_t ram_a, uint8_t command, uint8_t* data, uint8_t data_size);

//todo change + error check
esp_err_t etatherm_init()
{
	eta_uart_port = UART_NUM_2;
	uart_config_t uart_config = 
	{
		.baud_rate = 9600,
    	.data_bits = UART_DATA_8_BITS,
    	.parity = UART_PARITY_DISABLE,
    	.stop_bits = UART_STOP_BITS_1,
    	.flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
	};
	uart_param_config(eta_uart_port, &uart_config);
	uart_set_pin(eta_uart_port, 17, 16, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

	const int uart_buffer_size = (1024 * 2);
	uart_driver_install(eta_uart_port, uart_buffer_size, uart_buffer_size, 0, NULL, 0);
	return ESP_OK;
}

/// @brief get real temperature from device
/// @param uart_config
/// @param device_address in range from 0 to 15
/// @param out_value
/// @return 
etatherm_err_t etathermGetRealTemp(uint8_t device_address, uint8_t* out_value)
{
	uint8_t out_data[1];
	etatherm_err_t result;
	if (device_address > 15){
		return ETA_ERR_INVALID_ADDRESS;
	}
	result = _readData(1, 0x60 + device_address, out_data, 1);
	if(result){
		return result;
	}
	/// !!!!!
	if (device_address == 14){
		*out_value = 3 * (out_data[0] & 0xFF) + 5;
	} 
	else{
		*out_value = (out_data[0] & 0xFF)+ 5;
	}
	return ETA_OK;
}

etatherm_err_t etathermGetDesiredTemp(uint8_t device_address, uint8_t *out_value)
{
	uint8_t out_data[1];
	etatherm_err_t result;

	if (device_address > 15){
		return ETA_ERR_INVALID_ADDRESS;
	}
	result = _readData(1, 0x70 + device_address, out_data, 1);
	if(result){
		return result;
	}
	// or 0xFF ??
	// address 14 ?
	*out_value = (out_data[0] & 0x1F)+ 5;
	return ETA_OK;
}


// void etathermSetOZTemp(uint8_t Addr, uint8_t Temp)
// {
// 	Addr --;
// 	_readData(1, 0x1103 + (Addr << 4), (uint8_t*)&ETA_Data, 2);	
//     ETA_Data[0] = (ETA_Data[0] & 0xE0) | (Temp - 5);
//     ETA_Write(1, 0x1103 + (Addr << 4), (uint8_t*)&ETA_Data, 1);	
// }



// uint8_t ETA_Write(uint16_t BUSaddr, uint16_t RAMaddr, uint8_t* Data, uint8_t length)
// {
// 	uint8_t Command;
//     uint16_t Station;
//     uint16_t Response;
//     uint8_t Res;
//     uint8_t i;

//     Command = ((length - 1) << 4) | 0x0C;
//     for (i = 0; i < ETATHERM_RETRY; i++)
//     {
// 		_sendFrame(BUSaddr, RAMaddr, Command, Data, length);
// 		if (!(Res = _readFrame(2, &Station, (char*)&Response)))
// 		{
// 			if (!Response) return 0;	//Received O.K.
// 		}
//     }
//     return COMM_ERR_NOACK | Res;	//Write failed
// }


static etatherm_err_t _readData(uint16_t station_a, uint16_t ram_a, uint8_t* out_data, uint8_t data_size)
{
	uint8_t command = ((data_size - 1) << 4) | 0x08;
    etatherm_err_t result = 1;

    for (uint16_t i = 0; i < ETA_RETRY; i++)
    {
		result = uart_flush(eta_uart_port);
		if(result){
			continue;
		}
		ESP_LOGI("ETA", "buffer clear");
		result = _sendFrame(station_a, ram_a, command, out_data, data_size);
		if(result){
			continue;
		}
		ESP_LOGI("ETA", "frame sended");
		result = _readFrame(data_size, out_data);
		if (result){
			continue;
		}
		ESP_LOGI("ETA", "frame readed");
    }
	if(result){
		return result;
	}
	return ETA_OK;
}

static etatherm_err_t _sendFrame(uint16_t station_a, uint16_t ram_a, uint8_t command, uint8_t* data, uint8_t data_size)
{
	uint8_t ADDS = 0;
	uint8_t XORS = 0;
	uint8_t frame_buffer[data_size + 10];
	int32_t sended;

	frame_buffer[0] = DLE;
	frame_buffer[1] = SOH;
	frame_buffer[2] = station_a >> 8;
	frame_buffer[3] = station_a;
	frame_buffer[4] = ram_a >> 8;
	frame_buffer[5] = ram_a;
	frame_buffer[6] = command;
		
	memcpy(&frame_buffer + 7, data, data_size);
	
	for (int i = 0; i < data_size + 7; i++)				
	{
	    ADDS += frame_buffer[i];   			
		XORS ^= frame_buffer[i];
	}
	 
    frame_buffer[data_size + 7] = ADDS;
	frame_buffer[data_size + 8] = XORS;
	frame_buffer[data_size + 9] = 0xFF;

	sended = uart_write_bytes(eta_uart_port, frame_buffer, data_size + 10);
	if(sended != data_size + 10){
		return ETA_ERR_UART_WRITE;
	}
		for (int i = 0; i < data_size + 10; i++){
		ESP_LOGI("ETA", "0x%02X", frame_buffer[i]);
	}
	
	return ETA_OK;
}

static etatherm_err_t _readFrame(uint8_t data_size, uint8_t* out_data)
{
	data_size *= 2;
	// 0xFF 0xFF DLE ETB address D0 data(in WORDS) S0 S1 ADDS XORS
	uint8_t buffer[6 + data_size + 4];
	memset(buffer, 0, 4);

	uint8_t adds;
	uint8_t xors;
	uint16_t j = 0;
	int32_t received;

	int64_t end_time = esp_timer_get_time() + (ETA_FRAME_TIMEOUT_MIL * 1000);
	
	// find header
	while(true)
	{
		if(esp_timer_get_time() > end_time){
			return ETA_ERR_HEADER_READ_TIMEOUT;
		}
		received = uart_read_bytes(eta_uart_port, buffer, 1, ETA_BYTE_TIMEOUT_MIL / portTICK_PERIOD_MS);
		ESP_LOGI("ETA", "recived: %lu, read byte: 0x%02X", received, buffer[0]);
		if(received < 0){
			return ETA_ERR_UART_READ;
		}
		if((buffer[3] == 0xFF)&&(buffer[2] == 0xFF)&&(buffer[1] == DLE)&&(buffer[0] == ETB)){
			break;
		}
		printf("0x%02X 0x%02X 0x%02X 0x%02X\n", buffer[0], buffer[1],buffer[2],buffer[3]);
		buffer[3] = buffer[2];
		buffer[2] = buffer[1];
		buffer[1] = buffer[0];
		buffer[0] = 0;
	}
	// get rest of the frame
	received = uart_read_bytes(eta_uart_port, buffer + 4, 6 + data_size, ETA_FRAME_TIMEOUT_MIL / portTICK_PERIOD_MS);
	if(received !=  6 +data_size){
		return ETA_ERR_UART_READ;
	}

	buffer[3] = buffer[1];
	buffer[2] = buffer[0];

    //calculate checks
    adds = 0x00;
    xors = 0x00;  
    for (int i = 2; i < 6 + data_size; i++)
    { 
    	adds += buffer[i];
    	xors ^= buffer[i];  	
	} 

	if(buffer[6 + data_size + 2] != adds){
		return ETA_ERR_CRC_ADD;
	}
	if(buffer[6 + data_size + 3] != xors){
		return ETA_ERR_CRC_XOR;
	}

	//copy data, reduce WORDS -> BYTES
	for (int i = 6; i < 6 + data_size; i = i + 2)
	{
		out_data[j] = buffer[i];
		j++;
	}
	return ETA_OK;
}
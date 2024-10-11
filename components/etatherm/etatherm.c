#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "esp_timer.h"
#include "driver/uart.h"
//#include "esp_log.h"

#include "etatherm.h"

// TODO macro for results: ESP_RETURN_ON_ERROR

/** 
 * master -> slave communication (program -> etatherm controller),
 * frame structure (data payload: n bytes): 
 * B0: DLE
 * B1: SOH
 * B2 - B3: station (address bus)
 * B4 - B5: adress (address ram)
 * B6: command (DC)
 * B7 - B7+n: data (for: address - address+n)
 * B8+n: adds (ADD  B0 - B7+n)
 * B9+n: adds (XOR  B0 - B7+n)
 * B10+n: 0xFF
 */

/**
 * slave -> master communication (etatherm controller -> program)
 * frame structure (dat payload: n bytes): 
 * B0: 0xFF
 * B1: 0xFF
 * B3: DLE
 * B4: ETB
 * B4 - B5: station (address bus)
 * B6 - B6+2n: data (for: address - address+2n) ???
 * B7: S0 (response status) 0x00 - OK, 0x04 - fail
 * B8: S1 (response status) same as S0 ???
 * B9+2n: adds (ADD  B2 - B6+2n)
 * B10+2n: adds (XOR  B2 - B6+2n)
 */


// Protocol constatants
//
#define	DLE	0x10
#define	SOH	0x01
#define	ETB 0x17

// Communication constatants
//
#define ETA_FRAME_TIMEOUT_MIL 4000
#define ETA_BYTE_TIMEOUT_MIL 200
#define ETA_RETRY 1
 
 //
static uart_port_t eta_uart_port;

// Private functions
//
static uint8_t _GetTemp(uint16_t station, uint8_t device , uint16_t address, uint8_t *out_value);

static eta_err_t _ReadData(uint16_t station, uint16_t address, uint8_t* out_data, uint8_t data_size);
static uint8_t _WriteData(uint16_t station, uint16_t address, uint8_t* data, uint8_t data_size);

static eta_err_t _ReadFrame(uint8_t data_size, uint8_t* out_data);
static eta_err_t _SendFrame(uint16_t station, uint16_t address, uint8_t command, uint8_t* data, uint8_t data_size);

/**
 * @brief Configurate uart port and install driver
 * @param uart_port Uart port number
 * @param tx Rx pin number
 * @param rx Tx pin number
 * @return esp_err_t, ESP_OK Success - ESP_FAIL Parameter error
 */
esp_err_t eta_Init(uart_port_t uart_port, uint8_t tx, uint8_t rx)
{
	eta_uart_port = uart_port;
	esp_err_t result;
	uart_config_t uart_config = 
	{
		.baud_rate = 9600,
    	.data_bits = UART_DATA_8_BITS,
    	.parity = UART_PARITY_DISABLE,
    	.stop_bits = UART_STOP_BITS_1,
    	.flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
	};
	result = uart_param_config(eta_uart_port, &uart_config);
	if(result){
		return result;
	}
	result = uart_set_pin(eta_uart_port, tx, rx, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
		if(result){
		return result;
	}
	const int32_t uart_buffer_size = (1024 * 2);
	result = uart_driver_install(eta_uart_port, uart_buffer_size, uart_buffer_size, 0, NULL, 0);
		if(result){
		return result;
	}
	return ESP_OK;
}

/**
 * @brief Uninstall uart driver
 * @return esp_err_t, ESP_OK Success - ESP_FAIL Parameter error
 */
esp_err_t eta_Deinit(uart_port_t uart_port)
{
	esp_err_t result = uart_driver_delete(uart_port);
	return result;
}

/**
 * @brief Get real temperature from control head.
 * @param station Station in range 0 - 15.
 * @param device Device in range 0 - 15.
 * @param out_value 
 * @return eta_err_t, Returns ETA_OK (0) on success.
 */
eta_err_t eta_GetRealTemp(uint16_t station, uint8_t device, uint8_t* out_value)
{
	uint32_t address = 0x60 + device;
	eta_err_t result = _GetTemp(station, device, address, out_value);
	if(result){
		return result;
	}
	return ETA_OK;
}

/**
 * @brief Get desired temperature from control head.
 * @param station Station in range 0 - 15.
 * @param device Device in range 0 - 15.
 * @param out_value 
 * @return eta_err_t eta_err_t, Returns ETA_OK (0) on success.
 */
eta_err_t eta_GetDesiredTemp(uint16_t station, uint8_t device, uint8_t* out_value)
{
	uint32_t address = 0x70 + device;
	eta_err_t result = _GetTemp(station, device, address, out_value);
	if(result){
		return result;
	}
	return ETA_OK;
}

/**
 * @brief Get OZ temperature from etatherm controler.
 * @param station Station in range 0 - 15.
 * @param device Device in range 0 - 15.
 * @param out_value 
 * @return eta_err_t eta_err_t, Returns ETA_OK (0) on success.
 */
eta_err_t eta_GetOzTemp(uint16_t station, uint8_t device, uint8_t* out_value)
{
	uint32_t address = 0x1100 + 0x03 + (device << 4);
	eta_err_t result = _GetTemp(station, device, address, out_value);
	if(result){
		return result;
	}
	return ETA_OK;
}

/**
 * @brief Set OZ temperature on etatherm controller.
 * @param station Station in range 0 - 15.
 * @param device Station in range 0 - 15.
 * @param value Temperature value (max. 255)
 * @return eta_err_t, Returns ETA_OK (0) on success.
 */
eta_err_t eta_SetOzTemp( uint16_t station, uint8_t device, uint8_t value)
{
	uint8_t data[1];
	eta_err_t result;

	if (device > 15){
		return ETA_ERR_INVALID_DEVICE;
	}

	result = _ReadData(station, 0x1100 + 0x03 + (device << 4), data, 1);
	if (result){
		return result;
	}
	data[0] = (data[0] & 0xE0) | (value - 5);

	return _WriteData(station, 0x1100 + 0x03 + (device << 4), data, 1);
}

/**
 * @brief Get byte describing head temperature value shift.
 * 
 * b0 - b5: positive offest value
 * 
 * b6 - b7: multiplier 00 = 1, 01 = 2, 10 = 3, 11 = 4,
 * 
 * temperature value = multiplier * (x + offset)
 * 
 * @param station Station in range 0 - 15.
 * @param device Device in range 0 - 15.
 * @param out_value Output value.
 * @return eta_err_t Returns ETA_OK (0) on success.
 */
eta_err_t eta_GetTempShift(uint16_t station, uint8_t device, uint8_t *out_value)
{
	uint8_t out_data[1];
	eta_err_t result;
	if (device > 15){
		return ETA_ERR_INVALID_DEVICE;
	}
	if (station > 15){
		return ETA_ERR_INVALID_STATION;
	}
	result = _ReadData(station, 0x1100 + 0x02 + (device << 4), out_data, 1);
	if(result){
		return result;
	}
	*out_value = out_data[0];
	return ETA_OK;
}

static uint8_t _GetTemp(uint16_t station, uint8_t device , uint16_t address, uint8_t *out_value)
{
	uint8_t out_data[1];
	eta_err_t result;
	uint8_t shift = 0;

	if (device > 15){
		return ETA_ERR_INVALID_DEVICE;
	}
	if (station > 15){
		return ETA_ERR_INVALID_STATION;
	}
	result = _ReadData(station, address, out_data, 1);
	if(result){
		return result;
	}
	result = eta_GetTempShift(station, device, &shift);
	if(result){
		return result;
	}
	//calculate temperature from shift
	*out_value = ((shift >> 6) + 1) * (out_data[0] & 0x1F) + (shift & 0x3F);
	return ETA_OK;
}

/**
 * @brief Write n bytes to ram address of station
 */
static uint8_t _WriteData(uint16_t station, uint16_t address, uint8_t* data, uint8_t data_size)
{
	uint8_t command = ((data_size - 1) << 4) | 0x0C;
    uint8_t response[1];
	eta_err_t result = ETA_ERR;

    for (int32_t i = 0; i < ETA_RETRY; i++)
    {
		result = _SendFrame(station, address, command, data, data_size);
		if(result){
			continue;
		}
		result = uart_flush(eta_uart_port);
		if(result){
			result = ETA_ERR_UART_FLUSH;
			continue;
		}
		result = _ReadFrame(1, response);
		if (result){
			continue;
		}
		if(response[0] != 0){
			result = ETA_ERR_RESPONSE_FRAME_NOTNULL;
		}
		else{
			break;
		}
    }
	return result;
}

/**
 * @brief Read n bytes from ram address of station
 */
static eta_err_t _ReadData(uint16_t station, uint16_t address, uint8_t* out_data, uint8_t data_size)
{
	uint8_t command = ((data_size - 1) << 4) | 0x08;
    eta_err_t result = ETA_ERR;

    for (int32_t i = 0; i < ETA_RETRY; i++)
    {
		result = _SendFrame(station, address, command, out_data, data_size);
		if(result){
			continue;
		}
		result = uart_flush(eta_uart_port);
		if(result){
			result = ETA_ERR_UART_FLUSH;
			continue;
		}
		result = _ReadFrame(data_size, out_data);
		if (!result){
			break;
		}
    }
	return result;
}

static eta_err_t _SendFrame(uint16_t station, uint16_t address, uint8_t command, uint8_t* data, uint8_t data_size)
{
	uint8_t adds = 0;
	uint8_t xors = 0;
	uint8_t frame_buffer[data_size + 10];

	frame_buffer[0] = DLE;
	frame_buffer[1] = SOH;
	frame_buffer[2] = station >> 8;
	frame_buffer[3] = station;
	frame_buffer[4] = address >> 8;
	frame_buffer[5] = address;
	frame_buffer[6] = command;
	
	memcpy(frame_buffer + 7, data, data_size);
	
	for (int32_t i = 0; i < data_size + 7; i++)				
	{
	    adds += frame_buffer[i];   			
		xors ^= frame_buffer[i];
	}
	 
    frame_buffer[data_size + 7] = adds;
	frame_buffer[data_size + 8] = xors;
	frame_buffer[data_size + 9] = 0xFF;

	int32_t sended = uart_write_bytes(eta_uart_port, frame_buffer, data_size + 10);
	if(sended != data_size + 10){
		return ETA_ERR_UART_WRITE;
	}
	return ETA_OK;
}


static eta_err_t _ReadFrame(uint8_t data_size, uint8_t* out_data)
{
	data_size *= 2;
	// 0xFF 0xFF DLE ETB address D0 data(in WORDS) S0 S1 ADDS XORS
	uint8_t buffer[6 + data_size + 4];
	memset(buffer, 0, 4);

	uint8_t adds= 0x00;
	uint8_t xors= 0x00;
	uint32_t j = 0;
	int32_t received;

	int64_t end_time = esp_timer_get_time() + (ETA_FRAME_TIMEOUT_MIL * 1000);
	
	// find header
	while(true)
	{
		if(esp_timer_get_time() > end_time){
			return ETA_ERR_HEADER_READ_TIMEOUT;
		}
		received = uart_read_bytes(eta_uart_port, buffer, 1, ETA_BYTE_TIMEOUT_MIL / portTICK_PERIOD_MS);
		if(received < 0){
			return ETA_ERR_UART_READ;
		}
		if((buffer[3] == 0xFF)&&(buffer[2] == 0xFF)&&(buffer[1] == DLE)&&(buffer[0] == ETB)){
			break;
		}
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
    for (int32_t i = 2; i < 6 + data_size; i++)
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
	for (int32_t i = 6; i < 6 + data_size; i += 2)
	{
		out_data[j] = buffer[i];
		j++;
	}
	return ETA_OK;
}
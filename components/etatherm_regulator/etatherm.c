#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "driver/uart.h"

#include "etatherm.h"
		
#define	DLE	0x10
#define	SOH	0x01
#define	ETB 0x17

#define	ETATHERM_RETRY 3
#define ETATHERM_UART_NUM UART_NUM_2

#define BYTE_TIMEOUT 6//(96 / 16)  	//96 msec  
#define FRAME_TIMEOUT 8//(128 / 16)  //128 msec
#define FRAME_RESPONSE 2//(4 / 2)		//4 msec  

typedef enum
{
	ETATHERM_OK,
	ETATHERM_ERR,
	ETATHERM_ERR_IVALID_ADDRESS,
	ETATHERM_ERR_UART_WRITE
}etatherm_err_t;

/// @brief get real temperature from device
/// @param uart_config
/// @param device_address in range from 0 to 15
/// @param out_value
/// @return 
etatherm_err_t etathermGetRealTemp(uint8_t device_address, uint8_t* out_value)
{
	uint8_t data[1];
	if (device_address < 0 || device_address > 15){
		return ETATHERM_ERR_IVALID_ADDRESS;
	}
	_readData(1, 0x60 + device_address, data, 1);

	if (device_address == 14) itoa(pFOutStr, 3*(ETA_Data[0] & 0xFF) + 5, 10);	  //!! Added 21.12.2011 !!
    else itoa(pFOutStr, (ETA_Data[0] & 0xFF) + 5, 10);	
    return pFOutStr;
}

char* Get_DesTemp(char* pFOutStr, uint8_t Addr)
{
	Addr --;
	_readData(1, 0x70 + Addr, (uint8_t*)&ETA_Data, 2);
    itoa(pFOutStr, (ETA_Data[0] & 0x1F) + 5, 10);	
	return pFOutStr;	
}


void Set_OZ_Temp(uint8_t Addr, uint8_t Temp)
{
	Addr --;
	_readData(1, 0x1103 + (Addr << 4), (uint8_t*)&ETA_Data, 2);	
    ETA_Data[0] = (ETA_Data[0] & 0xE0) | (Temp - 5);
    ETA_Write(1, 0x1103 + (Addr << 4), (uint8_t*)&ETA_Data, 1);	
}



uint8_t ETA_Write(uint16_t BUSaddr, uint16_t RAMaddr, uint8_t* Data, uint8_t length)
{
	uint8_t Command;
    uint16_t Station;
    uint16_t Response;
    uint8_t Res;
    uint8_t i;

    Command = ((length - 1) << 4) | 0x0C;
    for (i = 0; i < ETATHERM_RETRY; i++)
    {
		_sendFrame(BUSaddr, RAMaddr, Command, Data, length);
		if (!(Res = Frame_receive(2, &Station, (char*)&Response)))
		{
			if (!Response) return 0;	//Received O.K.
		}
    }
    return COMM_ERR_NOACK | Res;	//Write failed
}


static etatherm_err_t _readData(uint16_t station_a, uint16_t ram_a, uint8_t* data, uint8_t data_size)
{
	uint8_t command = ((data_size - 1) << 4) | 0x08;
	uint8_t plain_data;
    uint8_t result;

    for (int i = 0; i < ETATHERM_RETRY; i++)
    {
		_sendFrame(station_a, ram_a, command, data, data_size);
		if (!(result = Frame_receive(station_a, ram_a, data))){
			return ETATHERM_OK;
		}
    }
	return result;
}


//xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
//			LOW LEVEL FUNCTIONS 
//xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
/****************************************************************************
			Send ETATHERM Protocol Frame (from Master)		  
			
****************************************************************************/
void _sendFrame(uint16_t station_a, uint16_t ram_a, uint8_t command, uint8_t* data, uint8_t data_size)
{
	uint8_t ADDS = 0;
	uint8_t XORS = 0;
	uint8_t frame_buffer[data_size + 8];

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

	int32_t sended = uart_write_bytes(ETATHERM_UART_NUM, frame_buffer, data_size + 8);
	if(sended < 0){
		return ETATHERM_ERR_UART_WRITE
	}
}

uint8_t	Frame_receive(uint8_t n, uint16_t* Address, uint8_t* Buffer)
{
	uint8_t	Ind = 0;
 	uint8_t status;
	uint8_t ADDS;
	uint8_t XORS;
	//		
//
	n = n << 1;  //Convert to byte count
	for (Ind = 0; Ind < 4; Ind ++) Frame_Buffer[Ind] = 0;
//  Find Header
    while ((Frame_Buffer[Ind] != 0xFF)||(Frame_Buffer[(Ind + 1) & 0x03] != 0xFF)||(Frame_Buffer[(Ind + 2) & 0x03] != DLE)||(Frame_Buffer[(Ind + 3) & 0x03] != ETB))
    {    	
    	if (Receive_Byte((uint8_t*)&Frame_Buffer + Ind, FRAME_TIMEOUT)) return FRAME_ERR_TIMEOUT;	//Timeout expired
    	Ind = (Ind + 1) & 0x03;    	
    }               
//  Receive Packet
	for (Ind = 0; Ind < n + 6; Ind++)
	{
		if (Receive_Byte((uint8_t*)&Frame_Buffer + Ind, BYTE_TIMEOUT)) return FRAME_ERR_TIMEOUT;	//Timeout expired 
    }    	
//	Calculate Checks
    ADDS = DLE + ETB;
    XORS = DLE ^ ETB;  
    for (Ind = 0; Ind < 2 + n; Ind++)
    { 
    	ADDS += Frame_Buffer[Ind];
    	XORS ^= Frame_Buffer[Ind];  	
	}    	
//	Address
   	*Address = (Frame_Buffer[0] << 8) + Frame_Buffer[1]; 
//  Data
    memcpy(Buffer,(uint8_t*)&Frame_Buffer + 2, n);
//  Status 1
    status = Frame_Buffer[2 + n];
//  Status 2
    //status = Frame_Buffer[3 + n];
//  ADDS Check
    if (Frame_Buffer[4 + n] != ADDS) return FRAME_ERR_ADDS;
//  XORS Check     	
    if (Frame_Buffer[5 + n] != XORS) return FRAME_ERR_XORS;
//
	return status;
}

uint8_t	Receive_Byte(uint8_t* data, uint8_t timeout)
{
	uint8_t CheckTime;
	//
	CheckTime = SleepTimer_1_bGetTickCntr() + timeout; 
	while (!(UART_1_bReadRxStatus() & UART_1_RX_REG_FULL))
	{
        if (SleepTimer_1_bGetTickCntr() == CheckTime)  return 1;	//Timeout
	}
	*data = UART_1_bReadRxData();
	return 0;		//received data  	
}
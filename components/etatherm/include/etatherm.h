typedef enum
{
	ETA_OK = 0,
	ETA_ERR,
	ETA_ERR_INVALID_ADDRESS,
	ETA_ERR_UART_WRITE,
	ETA_ERR_UART_READ,
	ETA_ERR_FRAME_READ_TIMEOUT,
	ETA_ERR_HEADER_READ_TIMEOUT,
	ETA_ERR_CRC_ADD,
	ETA_ERR_CRC_XOR
}etatherm_err_t;

etatherm_err_t etathermGetRealTemp(uint8_t device_address, uint8_t* out_value);
// etatherm_err_t etathermGetDesiredTemp(uint8_t device_address, uint8_t* out_value);

esp_err_t etatherm_init();
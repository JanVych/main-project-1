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
	ETA_ERR_CRC_XOR,
	ETA_ERR_RESPONSE_FRAME_NOTNULL
}eta_err_t;

eta_err_t etaGetRealTemp(uint8_t device_address, uint8_t* out_value);
eta_err_t etaGetDesiredTemp(uint8_t device_address, uint8_t* out_value);
eta_err_t etaGetOzTemp(uint8_t device_address, uint8_t *out_value);

eta_err_t etaSetOzTemp(uint8_t device_address, uint8_t value);

esp_err_t etaInit();
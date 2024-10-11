typedef enum
{
	ETA_OK = 0,
	ETA_ERR,
	ETA_ERR_INVALID_STATION,
	ETA_ERR_INVALID_DEVICE,
	ETA_ERR_FRAME_READ_TIMEOUT,
	ETA_ERR_HEADER_READ_TIMEOUT,
	ETA_ERR_CRC_ADD,
	ETA_ERR_CRC_XOR,
	ETA_ERR_RESPONSE_FRAME_NOTNULL,
	ETA_ERR_UART_FLUSH,
	ETA_ERR_UART_WRITE,
	ETA_ERR_UART_READ
}eta_err_t;

eta_err_t eta_GetRealTemp(uint16_t station, uint8_t device, uint8_t* out_value);
eta_err_t eta_GetDesiredTemp(uint16_t station, uint8_t device, uint8_t* out_value);
eta_err_t eta_GetOzTemp(uint16_t station, uint8_t device, uint8_t* out_value);

eta_err_t eta_SetOzTemp(uint16_t station, uint8_t device, uint8_t value);

eta_err_t eta_GetTempShift(uint16_t station, uint8_t device, uint8_t *out_value);

esp_err_t eta_Init(uart_port_t uart_port, uint8_t tx, uint8_t rx);
esp_err_t eta_Deinit(uart_port_t uart_port);
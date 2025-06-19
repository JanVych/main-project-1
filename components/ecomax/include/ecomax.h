#include "esp_err.h"
#include "driver/uart.h"

typedef struct ecomax_data_t {
    float mixTemperature;
    float flueTemperature;
    float tuvTemperature;
    float boilerTemperature;
    float acuUpperTemperature;
    float acuBottomTemperature;
    float outsideTemperature;
    float oxygenLevel;
    // int16_t fanPower;
} ecomax_data_t;

esp_err_t ecomax_Init(uart_port_t uart_num, int16_t tx, int32_t rx);
esp_err_t ecomax_Deinit(uart_port_t uart_num);
esp_err_t ecomax_PrintFrame();
esp_err_t ecomax_GetData();

// https://pyplumio.denpa.pro/protocol.html
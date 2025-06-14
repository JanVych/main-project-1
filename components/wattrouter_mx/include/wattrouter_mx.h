typedef enum 
{
    WATT_ROUTER_STATE_OFF = 0,
    WATT_ROUTER_STATE_ON = 1,
    WATT_ROUTER_STATE_AUTO = 2,
} watt_router_mx_state_t;

void wattrouter_Init();

esp_err_t wattrouter_GetTuvPower(int64_t* outValue);
esp_err_t wattrouter_GetTuvEnergy(int64_t* outValue);
esp_err_t wattrouter_GetAccuPower(int64_t* outValue);
esp_err_t wattrouter_GetAccuEnergy(int64_t* outValue);
esp_err_t wattrouter_GetFeedingPower(int32_t* outValue);

esp_err_t wattrouter_SetTuvState(watt_router_mx_state_t state);
esp_err_t wattrouter_SetBoilerState(watt_router_mx_state_t state);
esp_err_t wattrouter_SetFeedingPower(int32_t value);
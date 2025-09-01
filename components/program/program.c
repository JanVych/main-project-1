#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"

#include "server_comm.h"

#define LED1_GPIO GPIO_NUM_13
#define LED2_GPIO GPIO_NUM_14

#define BUTTON1_GPIO GPIO_NUM_26

static const char *_tag = "program";

typedef struct {
    gpio_num_t ledGpio;
    bool ledState;
    uint32_t intervalMs;
} LedBlinkParams_t;

static LedBlinkParams_t _ledBlinkParamsList[] = 
{
    { LED1_GPIO, false, 3000 }, // LED1, blink every 500ms
    { LED2_GPIO, false, 500}, // LED2, blink every 3s
};

static TaskHandle_t _led1TaskHandle = NULL;
static TaskHandle_t _led2TaskHandle = NULL;

static uint32_t _button1Pressed = 0;
TimerHandle_t _debounceTimer; 


static void BlinkLed(gpio_num_t gpioNum, uint8_t ledState) 
{
    gpio_set_level(gpioNum, ledState);
    ESP_LOGI(_tag, "LED on GPIO %d is now %s", gpioNum, ledState ? "ON" : "OFF");
}

static void LedBlinkTask(void *param)
{
    LedBlinkParams_t *p = (LedBlinkParams_t *)param;
    while (true) 
    {
        p->ledState = !p->ledState;
        BlinkLed(p->ledGpio, p->ledState);
        vTaskDelay(pdMS_TO_TICKS(p->intervalMs));
    }
}

void IRAM_ATTR ButtonIsrHandler(void *arg)
 {
    xTimerResetFromISR(_debounceTimer, NULL);
}

void debounceTimerCallback(TimerHandle_t Timer) 
{
    if (gpio_get_level(BUTTON1_GPIO) == 0) 
    {
        _button1Pressed++;
        ESP_LOGI(_tag, "Button Pressed!");
    }
}

void SetLedBlinkInterval(int ledNumber, uint32_t intervalMs) 
{
    if (ledNumber == 1)
    {
        _ledBlinkParamsList[0].intervalMs = intervalMs;
    } 
    else if (ledNumber == 2) 
    {
        _ledBlinkParamsList[1].intervalMs = intervalMs;
    } 
}

void ChangeLed1Interval(int32_t newInterval) 
{
    SetLedBlinkInterval(1, newInterval);
}

void ChangeLed2Interval(int32_t newInterval) 
{
    SetLedBlinkInterval(2, newInterval);
}

int32_t GetButtonPressCount() 
{
    int32_t count = _button1Pressed;
    _button1Pressed = 0;
    return count;
}

int32_t GetLed1Interval() 
{
    return _ledBlinkParamsList[0].intervalMs;
}
int32_t GetLed2Interval() 
{
    return _ledBlinkParamsList[1].intervalMs;
}

void OnProgramDestroy() 
{
    if (_led1TaskHandle != NULL) 
    {
        vTaskDelete(_led1TaskHandle);
    }
    if (_led2TaskHandle != NULL) 
    {
        vTaskDelete(_led2TaskHandle);
    }
    if (_debounceTimer != NULL) 
    {
        xTimerDelete(_debounceTimer, 0);
        gpio_isr_handler_remove(BUTTON1_GPIO);
    }
}
void Main()
{
    // Led configuration
    gpio_reset_pin(LED1_GPIO);
    gpio_set_direction(LED1_GPIO, GPIO_MODE_OUTPUT);
    gpio_reset_pin(LED2_GPIO);
    gpio_set_direction(LED2_GPIO, GPIO_MODE_OUTPUT);

    // led tasks
    xTaskCreate(LedBlinkTask, "Led2BlinkTask", 2048, &_ledBlinkParamsList[0], 5, &_led2TaskHandle);
    xTaskCreate(LedBlinkTask, "Led2BlinkTask", 2048, &_ledBlinkParamsList[1], 5, &_led2TaskHandle);

    // Button configuration
    gpio_set_direction(BUTTON1_GPIO, GPIO_MODE_INPUT);
    gpio_set_pull_mode(BUTTON1_GPIO, GPIO_PULLUP_ONLY); 
    gpio_set_intr_type(BUTTON1_GPIO, GPIO_INTR_NEGEDGE);

    _debounceTimer = xTimerCreate("debounceTimer", 
        pdMS_TO_TICKS(50), 
        pdFALSE, 
        NULL, 
        debounceTimerCallback);

    gpio_install_isr_service(0);
    gpio_isr_handler_add(BUTTON1_GPIO, ButtonIsrHandler, NULL);

    // Communication setup
    comm_AddMessageI32("ButtonPressCount", GetButtonPressCount);
    comm_AddMessageI32("Led1Interval", GetLed1Interval);
    comm_AddMessageI32("Led2Interval", GetLed2Interval);

    // LED control
    comm_AddActionInt32("SetLed1Interval", ChangeLed1Interval);
    comm_AddActionInt32("SetLed2Interval", ChangeLed2Interval);

    while (true)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}    
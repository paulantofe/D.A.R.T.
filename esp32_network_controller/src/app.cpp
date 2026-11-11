/**
 * @file     app.cpp
 * @brief    Application boot, UART monitoring, and failsafe task implementations.
 */

#include <Arduino.h>
#include "app.h"
#include "globals.h"
#include "server.h"
#include "camera.h"

#define UART_RX_PIN      

void Error_Handler(void) {
    Server_Stop();

    while (true) { delay(100); }
}


String rover_battery = "N/A";
volatile uint32_t last_command_time = 0;
volatile bool has_active_client = false;
volatile bool stm32_link_ok = false;
volatile bool stm32_link_status_known = false;

void ESP_Boot(void) {
    
}
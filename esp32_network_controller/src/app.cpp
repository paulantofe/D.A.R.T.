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

/**
 * @brief Monitor WiFi client status and trigger failsafe on command timeout.
 */
void Task_Network_Failsafe(uint32_t current_time) {
    
}

/**
 * @brief Send UART heartbeats and monitor STM32 acknowledgments.
 */
void Task_Hardware_Failsafe(uint32_t current_time) {
    
}

/**
 * @brief Process UART acknowledgments and battery messages without blocking.
 */
void Task_Process_UART_RX(void) {
    
}

/**
 * @file     app.h
 * @brief    Application boot and runtime tasks.
 */

#ifndef APP_H
#define APP_H

#include <Arduino.h>

void ESP_Boot(void);
void Task_Network_Failsafe(uint32_t current_time);
void Task_Hardware_Failsafe(uint32_t current_time);
void Task_Process_UART_RX(void);

#endif

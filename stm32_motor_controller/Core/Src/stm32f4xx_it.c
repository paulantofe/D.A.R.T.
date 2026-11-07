/**
 * @file     stm32f4xx_it.c
 * @brief    Interrupts source file.
 *
 * @details  This file implements the Service Routines for different IRQs.
 */

#include "stm32f4xx_hal_hardware_config.h"
#include "stm32f4xx_it.h"

void SysTick_Handler(void) {
	HAL_IncTick();
}

void USART1_IRQHandler(void) {
	HAL_UART_IRQHandler(&huart1);
}

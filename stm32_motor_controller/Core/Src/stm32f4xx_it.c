/**
 * @file     stm32f4xx_it.c
 * @brief    Interrupts source file.
 *
 * @details  This file implements the Service Routines for different IRQs.
 */

#include "stm32f4xx_it.h"
#include "stm32f4xx_hal.h"
#include "board_peripherals.h"

void SysTick_Handler(void) {
	HAL_IncTick();
}

void USART1_IRQHandler(void) {
	HAL_UART_IRQHandler(&huart1);
}

void TIM6_DAC_IRQHandler(void) {
	HAL_TIM_IRQHandler(&htim6);
}

void DMA1_Stream7_IRQHandler(void) {
	HAL_DMA_IRQHandler(&hdma1);
}

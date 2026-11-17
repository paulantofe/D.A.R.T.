/**
 * @file     stm32f4xx_hal_callbacks.c
 * @brief    STM32 HAL callback implementations.
 */

#include "globals.h"
#include "rdt_link.h"
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_hardware_config.h"

/* ------------------ UART Receive Callback ------------------ */

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
	if (huart->Instance != USART1) {
		return;
	}

	RDT_Handle_IncomingByte(received_byte);

	HAL_UART_Receive_IT(huart, &received_byte, 1);
}

/* ------------------------------------------------------------ */


/* ------------------ UART Transmit Callback ------------------ */

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart) {
	if (huart->Instance != USART1) {
		return;
	}

	RDT_Reset_TxBuffer();
}

/* ------------------------------------------------------------ */

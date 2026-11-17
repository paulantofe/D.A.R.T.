#include "stm32f4xx_hal_hardware_config.h"
#include "globals.h"
#include "rdt_link.h"

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
	if (huart->Instance != USART1) {
		return;
	}

	RDT_Handle_IncomingByte(received_byte);

	HAL_UART_Receive_IT(huart, &received_byte, 1);
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart) {
	if (huart->Instance != USART1) {
		return;
	}

	Reset_TBuffer();
}

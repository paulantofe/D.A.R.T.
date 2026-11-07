#include "stm32f4xx_hal_hardware_config.h"
#include "globals.h"

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
	if (huart->Instance != USART1) { return; }

	const uint8_t received = rx_byte;
	const uint8_t head = rx_head;
	const uint8_t next = (uint8_t) (head + 1U) % RX_BUFFER_CAPACITY;

	if (next == rx_tail) {
		rx_overrun = 1U;
	} else {
		rx_buffer[head] = received;
		rx_head = next;
	}

	if (HAL_UART_Receive_IT(huart, &rx_byte, 1U) != HAL_OK) {
		rx_error = 1U;
	}
}

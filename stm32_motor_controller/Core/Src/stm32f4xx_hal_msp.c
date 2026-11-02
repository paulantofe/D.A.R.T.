#include "main.h"

void HAL_MspInit(void) {
	__HAL_RCC_SYSCFG_CLK_ENABLE();
	__HAL_RCC_PWR_CLK_ENABLE();
}

void HAL_UART_MspInit(UART_HandleTypeDef *huart) {
	if (huart->Instance == USART1) {
		__HAL_RCC_USART1_CLK_ENABLE();
		__HAL_RCC_GPIOA_CLK_ENABLE();

		GPIO_InitTypeDef uart_pin = { 0 };
		uart_pin.Mode = GPIO_MODE_AF_PP;
		uart_pin.Alternate = GPIO_AF7_USART1;
		uart_pin.Pull = GPIO_PULLUP;
		uart_pin.Speed = GPIO_SPEED_FAST;
        uart_pin.Pin = GPIO_PIN_9 | GPIO_PIN_10;
		HAL_GPIO_Init(GPIOA, &uart_pin);

		HAL_NVIC_EnableIRQ(USART1_IRQn);
		HAL_NVIC_SetPriority(USART1_IRQn, 5, 0);
	}
}

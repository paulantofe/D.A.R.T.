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

void HAL_TIM_Base_MspInit(TIM_HandleTypeDef *htim) {
	if (htim->Instance == TIM6) {
		__HAL_RCC_TIM6_CLK_ENABLE();

		HAL_NVIC_EnableIRQ(TIM6_DAC_IRQn);
		HAL_NVIC_SetPriority(TIM6_DAC_IRQn, 4, 0);
	}
}

void HAL_TIM_PWM_MspInit(TIM_HandleTypeDef *htim) {
	if (htim->Instance == TIM3) {
		__HAL_RCC_TIM3_CLK_ENABLE();
		__HAL_RCC_GPIOA_CLK_ENABLE();

		GPIO_InitTypeDef tim3_ch = { 0 };
		tim3_ch.Mode = GPIO_MODE_AF_PP;
		tim3_ch.Alternate = GPIO_AF2_TIM3;
		tim3_ch.Pull = GPIO_NOPULL;
		tim3_ch.Speed = GPIO_SPEED_FAST;
		tim3_ch.Pin = GPIO_PIN_6 | GPIO_PIN_7;

		HAL_GPIO_Init(GPIOA, &tim3_ch);
	}
}

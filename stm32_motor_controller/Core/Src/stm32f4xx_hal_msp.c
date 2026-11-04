/**
 * @file     stm32f4xx_hal_msp.c
 * @brief    MSP Config source file.
 *
 * @details  This file implements the MSP APIs called by HAL Initialization APIs by configuring
 *           the low-level hardware (Clocks, GPIO Pins, NVIC IRQ Enable / Priority Set).
 */

#include "stm32f4xx_hal.h"

void HAL_MspInit(void) {
	__HAL_RCC_SYSCFG_CLK_ENABLE();
	__HAL_RCC_PWR_CLK_ENABLE();

	HAL_NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4);
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

void HAL_TIM_Encoder_MspInit(TIM_HandleTypeDef *htim) {
	if (htim->Instance == TIM2) {
		__HAL_RCC_TIM2_CLK_ENABLE();
		__HAL_RCC_GPIOA_CLK_ENABLE();
		__HAL_RCC_GPIOB_CLK_ENABLE();

		GPIO_InitTypeDef tim2_ch = { 0 };
		tim2_ch.Mode = GPIO_MODE_AF_PP;
		tim2_ch.Alternate = GPIO_AF1_TIM2;
		tim2_ch.Pull = GPIO_NOPULL;
		tim2_ch.Speed = GPIO_SPEED_FAST;

		tim2_ch.Pin = GPIO_PIN_15;
		HAL_GPIO_Init(GPIOA, &tim2_ch);

		tim2_ch.Pin = GPIO_PIN_3;
        HAL_GPIO_Init(GPIOB, &tim2_ch);
	}
	else if (htim->Instance == TIM5) {
		__HAL_RCC_TIM5_CLK_ENABLE();
		__HAL_RCC_GPIOA_CLK_ENABLE();

		GPIO_InitTypeDef tim5_ch = { 0 };
		tim5_ch.Mode = GPIO_MODE_AF_PP;
		tim5_ch.Alternate = GPIO_AF2_TIM5;
		tim5_ch.Pull = GPIO_NOPULL;
		tim5_ch.Speed = GPIO_SPEED_FAST;
		tim5_ch.Pin = GPIO_PIN_0 | GPIO_PIN_1;

		HAL_GPIO_Init(GPIOA, &tim5_ch);

	}
}

void HAL_ADC_MspInit(ADC_HandleTypeDef *hadc) {
	if (hadc->Instance == ADC1) {
		__HAL_RCC_ADC1_CLK_ENABLE();
		__HAL_RCC_GPIOA_CLK_ENABLE();

		GPIO_InitTypeDef adc_ch = { 0 };
		adc_ch.Mode = GPIO_MODE_ANALOG;
		adc_ch.Pull = GPIO_NOPULL;
		adc_ch.Pin = GPIO_PIN_3;

		HAL_GPIO_Init(GPIOA, &adc_ch);
	}
}

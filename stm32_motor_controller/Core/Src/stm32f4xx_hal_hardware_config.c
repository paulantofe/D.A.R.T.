#include "stm32f4xx_hal_hardware_config.h"

UART_HandleTypeDef huart1;
TIM_HandleTypeDef htim6;
TIM_HandleTypeDef htim3;
TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim5;

void STM_Boot(void) {
	// for later...
}

void Error_Handler(void) {
	__disable_irq();

	while (1) {
		HAL_GPIO_TogglePin(GPIOD, GPIO_PIN_14);
		for (volatile uint32_t i = 0; i < 2000000; i++) { __NOP(); }
	}
}

void SystemClock_Config(void) {
	RCC_OscInitTypeDef osc_init = { 0 };
	RCC_ClkInitTypeDef clk_init = { 0 };

	__HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

	__HAL_FLASH_PREFETCH_BUFFER_ENABLE();
	__HAL_FLASH_DATA_CACHE_ENABLE();
	__HAL_FLASH_INSTRUCTION_CACHE_ENABLE();

	osc_init.OscillatorType = RCC_OSCILLATORTYPE_HSE;
	osc_init.HSEState = RCC_HSE_ON;
	osc_init.PLL.PLLSource = RCC_PLLSOURCE_HSE;
	osc_init.PLL.PLLState = RCC_PLL_ON;
	osc_init.PLL.PLLM = 4;
	osc_init.PLL.PLLN = 168;
	osc_init.PLL.PLLP = RCC_PLLP_DIV2;
	osc_init.PLL.PLLQ = 7;
	if (HAL_RCC_OscConfig(&osc_init) != HAL_OK) {
		Error_Handler();
	}

	clk_init.ClockType = RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
	clk_init.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	clk_init.AHBCLKDivider = RCC_SYSCLK_DIV1;
	clk_init.APB1CLKDivider = RCC_HCLK_DIV4;
	clk_init.APB2CLKDivider = RCC_HCLK_DIV2;
	if (HAL_RCC_ClockConfig(&clk_init, FLASH_LATENCY_5) != HAL_OK) {
		Error_Handler();
	}
}

void LED_Config(void) {
	GPIO_InitTypeDef led = { 0 };

	__HAL_RCC_GPIOD_CLK_ENABLE();

	led.Mode = GPIO_MODE_OUTPUT_PP;
	led.Pull = GPIO_NOPULL;
	led.Speed = GPIO_SPEED_MEDIUM;
	led.Pin = GPIO_PIN_13 | GPIO_PIN_14;

	HAL_GPIO_Init(GPIOD, &led);
}

void UART_Config(void) {
	huart1.Instance = USART1;
	huart1.Init.BaudRate = 115200;
	huart1.Init.Mode = UART_MODE_TX_RX;
	huart1.Init.WordLength = UART_WORDLENGTH_8B;
	huart1.Init.Parity = UART_PARITY_NONE;
	huart1.Init.StopBits = UART_STOPBITS_1;
	huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
	huart1.Init.OverSampling = UART_OVERSAMPLING_16;

	if (HAL_UART_Init(&huart1) != HAL_OK) {
		Error_Handler();
	}
}

void TIM6_Config(void) {
	htim6.Instance = TIM6;
	htim6.Init.Prescaler = 8400 - 1;
	htim6.Init.CounterMode = TIM_COUNTERMODE_UP;
	htim6.Init.Period = 100 - 1;
	htim6.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;

	if (HAL_TIM_Base_Init(&htim6) != HAL_OK) {
		Error_Handler();
	}
}

void TIM3_Config(void) {
	TIM_OC_InitTypeDef tim3PWM_Config = { 0 };

	htim3.Instance = TIM3;
	htim3.Init.Prescaler = 0;
	htim3.Init.Period = 4200 - 1;
	htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
	htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
	if (HAL_TIM_PWM_Init(&htim3) != HAL_OK) {
		Error_Handler();
	}

	tim3PWM_Config.OCMode = TIM_OCMODE_PWM1;
	tim3PWM_Config.OCPolarity = TIM_OCPOLARITY_HIGH;
	tim3PWM_Config.Pulse = 0;
	if (HAL_TIM_PWM_ConfigChannel(&htim3, &tim3PWM_Config, TIM_CHANNEL_1) != HAL_OK) {
		Error_Handler();
	}
	if (HAL_TIM_PWM_ConfigChannel(&htim3, &tim3PWM_Config, TIM_CHANNEL_2) != HAL_OK) {
		Error_Handler();
	}
}

void TIM2_Config(void) {
	TIM_Encoder_InitTypeDef sConfig = { 0 };

	htim2.Instance = TIM2;
	htim2.Init.Prescaler = 0;
	htim2.Init.Period = 0xFFFFFFFF;
	htim2.Init.CounterMode = TIM_COUNTERMODE_UP;

	sConfig.EncoderMode = TIM_ENCODERMODE_TI12;
	sConfig.IC1Polarity = TIM_ICPOLARITY_RISING;
	sConfig.IC1Selection = TIM_ICSELECTION_DIRECTTI;
	sConfig.IC1Prescaler = TIM_ICPSC_DIV1;
	sConfig.IC1Filter = 10;
	sConfig.IC2Polarity = TIM_ICPOLARITY_RISING;
	sConfig.IC2Selection = TIM_ICSELECTION_DIRECTTI;
	sConfig.IC2Prescaler = TIM_ICPSC_DIV1;
	sConfig.IC2Filter = 10;

	if (HAL_TIM_Encoder_Init(&htim2, &sConfig) != HAL_OK) {
		Error_Handler();
	}
}

void TIM5_Config(void) {

}

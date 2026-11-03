#include "stm32f4xx_hal_hardware_config.h"

void Error_Handler(void) {
	__disable_irq();

	while (1) {
		HAL_GPIO_TogglePin(GPIOD, GPIO_PIN_14);
		for (volatile uint32_t i = 0; i < 2000000; i++) { __NOP(); }
	}
}

int main(void) {
	STM_Boot();

	while (1) {

	}
}




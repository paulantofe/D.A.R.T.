#include "stm32f4xx_hal_hardware_config.h"
#include "stm32f4xx_it.h"

void SysTick_Handler(void) {
	HAL_IncTick();
}

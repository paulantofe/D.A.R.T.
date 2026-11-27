#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_hardware_config.h"
#include "lighting_service.h"

void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim) {
	if (htim->Instance == TIM4) {
		LS_OnDmaComplete();
	}
}

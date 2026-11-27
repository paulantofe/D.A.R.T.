#include "fault_service.h"
#include "stm32f4xx_hal_hardware_config.h"

HAL_StatusTypeDef F_Init(F_Handle_t *hf) {
	if (hf == NULL) {
		return HAL_ERROR;
	}

	hf->fault_state = F_State_Normal;
	return HAL_OK;
}

HAL_StatusTypeDef F_EnterFailsafe(F_Handle_t *hf) {
	if (hf == NULL || hf->fault_state == F_State_Systemfault) {
		return HAL_ERROR;
	}

	hf->fault_state = F_State_Failsafe;
	return HAL_OK;
}

HAL_StatusTypeDef F_ExitFailsafe(F_Handle_t *hf) {
	if (hf == NULL || hf->fault_state == F_State_Systemfault) {
		return HAL_ERROR;
	}

	hf->fault_state = F_State_Normal;
	return HAL_OK;
}

void F_EnterSystemfault(F_Handle_t *hf) {
	if (hf != NULL) {
		hf->fault_state = F_State_Systemfault;
	}
}

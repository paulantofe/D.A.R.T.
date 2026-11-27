#include "fault_service.h"

/* ------------------ Static Service Instance ------------------ */

typedef enum {
	F_State_Normal = 0,
	F_State_Failsafe,
	F_State_Systemfault
} F_State_t;

typedef struct {
	F_State_t fault_state;
} F_Handle_t;

static F_Handle_t fault_service;

/* ------------------------------------------------------------- */

HAL_StatusTypeDef F_Init(void) {
	fault_service.fault_state = F_State_Normal;
	return HAL_OK;
}

HAL_StatusTypeDef F_EnterFailsafe(void) {
	if (fault_service.fault_state == F_State_Systemfault) {
		return HAL_ERROR;
	}

	fault_service.fault_state = F_State_Failsafe;
	return HAL_OK;
}

HAL_StatusTypeDef F_ExitFailsafe(void) {
	if (fault_service.fault_state == F_State_Systemfault) {
		return HAL_ERROR;
	}

	fault_service.fault_state = F_State_Normal;
	return HAL_OK;
}

void F_EnterSystemfault(void) {
	fault_service.fault_state = F_State_Systemfault;
}

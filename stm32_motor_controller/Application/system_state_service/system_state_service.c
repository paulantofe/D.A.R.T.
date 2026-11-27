/**
 * @file    system_state_service.c
 * @brief   System-state service implementation.
 * @details Implements application-state requests and state transitions used
 *          by the rover super loop.
 */

#include "system_state_service.h"
#include "fault_service.h"

/* ------------------ Static Helper Functions ------------------ */

typedef struct {
	SS_State_t system_state;
	bool is_initialized;
	volatile bool request_ss_change;
	volatile SS_State_t requested_state;
	volatile bool was_in_failsafe;
	volatile bool sysfault_requested;
} SS_Handle_t;

static SS_Handle_t system_state_service;

/**
 * @brief Validate a system-state value.
 * @param[in] state State value to validate.
 * @retval true State is valid.
 * @retval false State is not defined by the service.
 */
static inline bool is_valid_state(const SS_State_t state) {
	switch (state) {
	case SS_State_Boot:
	case SS_State_Failsafe:
	case SS_State_Idle:
	case SS_State_Moving:
	case SS_State_Systemfault:
		return true;
	default:
		return false;
	}
}

/* ------------------------------------------------------------- */


/* ------------------ Public APIs Implementation ------------------ */

HAL_StatusTypeDef SS_Init(void) {
	system_state_service.system_state = SS_State_Boot;
	system_state_service.requested_state = SS_State_Boot;
	system_state_service.sysfault_requested = false;
	system_state_service.request_ss_change = false;
	system_state_service.was_in_failsafe = false;
	system_state_service.is_initialized = true;

	return HAL_OK;
}

HAL_StatusTypeDef SS_RequestState(const SS_State_t state) {
	if (!system_state_service.is_initialized || !is_valid_state(state)) {
		return HAL_ERROR;
	}

	uint32_t primask = __get_PRIMASK();
	__disable_irq();

	if (state != SS_State_Systemfault
			&& (system_state_service.sysfault_requested
					|| system_state_service.system_state == SS_State_Systemfault)) {
		__set_PRIMASK(primask);
		return HAL_ERROR;
	}

	system_state_service.requested_state = state;
	system_state_service.request_ss_change = true;
	if (state == SS_State_Systemfault) {
		system_state_service.sysfault_requested = true;
	}

	__set_PRIMASK(primask);

	return HAL_OK;
}

HAL_StatusTypeDef SS_GetState(SS_State_t *state) {
	if (state == NULL || !system_state_service.is_initialized) {
		return HAL_ERROR;
	}

	*state = system_state_service.system_state;

	return HAL_OK;
}

HAL_StatusTypeDef SS_Process(void) {
	if (!system_state_service.is_initialized) {
		return HAL_ERROR;
	}

	uint32_t primask = __get_PRIMASK();
	__disable_irq();

	bool fault_requested = system_state_service.sysfault_requested;
	bool request_pending = system_state_service.request_ss_change;
	SS_State_t requested_state = system_state_service.requested_state;
	system_state_service.request_ss_change = false;

	if (!fault_requested && !request_pending) {
		__set_PRIMASK(primask);
		return HAL_OK;
	}

	__set_PRIMASK(primask);

	if (fault_requested || requested_state == SS_State_Systemfault) {
		if (system_state_service.system_state != SS_State_Systemfault) {
			F_EnterSystemfault();
			system_state_service.system_state = SS_State_Systemfault;
			system_state_service.was_in_failsafe = false;
		}
		return HAL_OK;
	}

	if (system_state_service.system_state == SS_State_Systemfault) {
		return HAL_ERROR;
	}

	switch (requested_state) {
	case SS_State_Boot:
		system_state_service.system_state = SS_State_Boot;
		break;

	case SS_State_Failsafe:
		if (system_state_service.system_state != SS_State_Failsafe) {
			if (F_EnterFailsafe() != HAL_OK) {
				return HAL_ERROR;
			}
		}

		system_state_service.system_state = SS_State_Failsafe;
		system_state_service.was_in_failsafe = true;
		break;

	case SS_State_Idle:
		if (system_state_service.was_in_failsafe) {
			if (F_ExitFailsafe() != HAL_OK) {
				return HAL_ERROR;
			}
			system_state_service.was_in_failsafe = false;
		}

		system_state_service.system_state = SS_State_Idle;
		break;

	case SS_State_Moving:
		if (system_state_service.was_in_failsafe) {
			return HAL_ERROR;
		}

		system_state_service.system_state = SS_State_Moving;
		break;

	case SS_State_Systemfault:
		return HAL_ERROR;

	default:
		return HAL_ERROR;
	}

	return HAL_OK;
}

/* ---------------------------------------------------------------- */

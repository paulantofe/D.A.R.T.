/**
 * @file    system_state_service.c
 * @brief   System-state service implementation.
 * @details Implements application-state requests and state transitions used
 *          by the rover super loop.
 */

#include "system_state_service.h"
#include "fault_service.h"

/* ------------------ Static Helper Functions ------------------ */

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

HAL_StatusTypeDef SS_Init(SS_Handle_t *hss) {
	if (hss == NULL) {
		return HAL_ERROR;
	}

	hss->system_state = SS_State_Boot;
	hss->requested_state = SS_State_Boot;
	hss->request_ss_change = false;
	hss->was_in_failsafe = false;
	hss->is_initialized = true;

	return HAL_OK;
}

HAL_StatusTypeDef SS_RequestState(SS_Handle_t *hss, const SS_State_t state) {
	if (hss == NULL || !hss->is_initialized || !is_valid_state(state)) {
		return HAL_ERROR;
	}

	uint32_t primask = __get_PRIMASK();
	__disable_irq();

	hss->requested_state = state;
	hss->request_ss_change = true;

	__set_PRIMASK(primask);

	return HAL_OK;
}

HAL_StatusTypeDef SS_GetState(SS_Handle_t *hss, SS_State_t *state) {
	if (hss == NULL || state == NULL || !hss->is_initialized) {
		return HAL_ERROR;
	}

	*state = hss->system_state;

	return HAL_OK;
}

HAL_StatusTypeDef SS_Process(SS_Handle_t *hss) {
	if (hss == NULL || !hss->is_initialized) {
		return HAL_ERROR;
	}

	uint32_t primask = __get_PRIMASK();
	__disable_irq();

	if (!hss->request_ss_change) {
		__set_PRIMASK(primask);
		return HAL_OK;
	}

	SS_State_t requested_state = hss->requested_state;
	hss->request_ss_change = false;

	__set_PRIMASK(primask);

	switch (requested_state) {
	case SS_State_Boot:
		hss->system_state = SS_State_Boot;
		break;

	case SS_State_Failsafe:
		if (hss->system_state == SS_State_Systemfault) {
			return HAL_ERROR;
		}

		if (hss->system_state != SS_State_Failsafe) {
			F_EnterFailsafe();
		}

		hss->system_state = SS_State_Failsafe;
		hss->was_in_failsafe = true;
		break;

	case SS_State_Idle:
		if (hss->system_state == SS_State_Systemfault) {
			return HAL_ERROR;
		}

		if (hss->was_in_failsafe) {
			hss->was_in_failsafe = false;
			F_ExitFailsafe();
		}

		hss->system_state = SS_State_Idle;
		break;

	case SS_State_Moving:
		if (hss->system_state == SS_State_Systemfault
				|| hss->was_in_failsafe) {
			return HAL_ERROR;
		}

		hss->system_state = SS_State_Moving;
		break;

	case SS_State_Systemfault:
		if (hss->system_state != SS_State_Systemfault) {
			hss->system_state = SS_State_Systemfault;
			F_EnterSystemFault();
		}
		break;

	default:
		return HAL_ERROR;
	}

	return HAL_OK;
}

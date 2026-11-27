/**
 * @file    fault_service.c
 * @brief   Fault-state management and safety lighting integration.
 * @details Implements the singleton fault service. Fault transitions are
 *          serialized through the service state and use bounded retries when
 *          the lighting service is busy.
 */

#include "fault_service.h"
#include "lighting_service.h"
#include <stdbool.h>

/* ------------------ General Definitions ------------------ */

#define F_LIGHTING_TIMEOUT_MS 100U /**< Maximum time spent retrying lighting. */

/* ----------------------------------------------------------- */


/* ------------------ Static Service Instance ------------------ */

/** Fault states owned by the singleton fault service. */
typedef enum {
	F_State_Normal = 0,
	F_State_Failsafe,
	F_State_Systemfault
} F_State_t;

/** Internal state of the singleton fault service. */
typedef struct {
	F_State_t fault_state;  /**< Current fault state. */
	bool is_initialized;    /**< True after successful service initialization. */
} F_Handle_t;

static F_Handle_t fault_service; /**< Singleton fault-service instance. */

/* ------------------------------------------------------------- */

/* ------------------ Static Helper Functions ------------------ */

/**
 * @brief Apply a lighting state with bounded busy retries.
 * @param[in] state System state to indicate through the lighting service.
 * @retval HAL_OK      Lighting state applied successfully.
 * @retval HAL_ERROR   Lighting service rejected the request.
 * @retval HAL_TIMEOUT Lighting service remained busy until the timeout.
 */
static HAL_StatusTypeDef F_SetLightingState(const SS_State_t state) {
	const uint32_t start_ms = HAL_GetTick();
	HAL_StatusTypeDef status;

	do {
		status = LS_SetStateColor(state);
		if (status != HAL_BUSY) {
			return status;
		}

		(void)LS_Process();
	} while ((HAL_GetTick() - start_ms) < F_LIGHTING_TIMEOUT_MS);

	return HAL_TIMEOUT;
}

/* ------------------------------------------------------------- */


/* ------------------ Public APIs ------------------ */

HAL_StatusTypeDef F_Init(void) {
	fault_service.fault_state = F_State_Normal;
	fault_service.is_initialized = true;
	return HAL_OK;
}

HAL_StatusTypeDef F_EnterFailsafe(void) {
	if (fault_service.fault_state == F_State_Systemfault
			|| !fault_service.is_initialized) {
		return HAL_ERROR;
	}

	fault_service.fault_state = F_State_Failsafe;
	return F_SetLightingState(SS_State_Failsafe);
}

HAL_StatusTypeDef F_ExitFailsafe(void) {
	if (fault_service.fault_state == F_State_Systemfault
			|| !fault_service.is_initialized) {
		return HAL_ERROR;
	}

	fault_service.fault_state = F_State_Normal;
	return F_SetLightingState(SS_State_Idle);
}

void F_EnterSystemfault(void) {
	fault_service.fault_state = F_State_Systemfault;
	(void)F_SetLightingState(SS_State_Systemfault);
}

/* ------------------------------------------------- */

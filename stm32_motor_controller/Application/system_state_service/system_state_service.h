/**
 * @file    system_state_service.h
 * @brief   Public API and data structures for the system-state service.
 * @details Provides state initialization, state requests, state processing,
 *          and current-state access for the application super loop.
 */

#ifndef SYSTEM_STATE_SERVICE_SYSTEM_STATE_SERVICE_H_
#define SYSTEM_STATE_SERVICE_SYSTEM_STATE_SERVICE_H_

#include "stm32f4xx_hal.h"
#include <stdbool.h>

/* ------------------ General Definitions ------------------ */

/* Application-level states handled by the system-state service */
typedef enum {
	SS_State_Boot = 0, /**< System startup and initialization state. */
	SS_State_Idle, /**< System initialized and not moving. */
	SS_State_Moving, /**< Rover motion is enabled. */
	SS_State_Failsafe, /**< Safety response to a recoverable fault condition. */
	SS_State_Systemfault /**< Fatal or latched system fault state. */
} SS_State_t;

/* ----------------------------------------------------------- */


/* ------------------ Structures Definition ------------------ */

/* Runtime handle for the system-state service */
typedef struct {
	SS_State_t system_state;             /**< Currently applied system state. */
	bool is_initialized;                 /**< True after successful service initialization. */
	volatile bool request_ss_change;     /**< True when a state request is pending. */
	volatile SS_State_t requested_state; /**< State requested by the producer. */
	volatile bool was_in_failsafe;       /**< True until the failsafe state is exited. */
} SS_Handle_t;

/* ----------------------------------------------------------- */

/* ------------------ Public APIs ------------------ */

/**
 * @brief Initialize the system-state service.
 * @param[in,out] hss Pointer to the service handle.
 * @retval HAL_OK Service initialized in the boot state.
 * @retval HAL_ERROR NULL handle.
 */
HAL_StatusTypeDef SS_Init(SS_Handle_t *hss);

/**
 * @brief Request a transition to a new system state.
 * @param[in,out] hss Pointer to an initialized service handle.
 * @param[in] state State to be applied by SS_Process().
 * @retval HAL_OK Request recorded successfully.
 * @retval HAL_ERROR NULL or uninitialized handle, or invalid state.
 * @note This API protects the state request and pending flag as one
 *       critical operation and may be called from interrupt context.
 */
HAL_StatusTypeDef SS_RequestState(SS_Handle_t *hss, const SS_State_t state);

/**
 * @brief Read the currently applied system state.
 * @param[in] hss Pointer to an initialized service handle.
 * @param[out] state Destination for the current state.
 * @retval HAL_OK State returned successfully.
 * @retval HAL_ERROR NULL handle, uninitialized handle, or NULL state pointer.
 */
HAL_StatusTypeDef SS_GetState(SS_Handle_t *hss, SS_State_t *state);

/**
 * @brief Apply one pending state request.
 * @param[in,out] hss Pointer to an initialized service handle.
 * @retval HAL_OK No request was pending or the request was applied.
 * @retval HAL_ERROR NULL or uninitialized handle, or invalid transition.
 * @note Call this API from the application super loop. State transitions and
 *       fault-service actions execute outside the request critical section.
 */
HAL_StatusTypeDef SS_Process(SS_Handle_t *hss);

/* ------------------------------------------------- */

#endif /* SYSTEM_STATE_SERVICE_SYSTEM_STATE_SERVICE_H_ */

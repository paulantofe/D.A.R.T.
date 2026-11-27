/**
 * @file    fault_service.h
 * @brief   Public API for the rover fault service.
 * @details Provides a singleton service for managing normal operation,
 *          failsafe operation, and latched system-fault behavior.
 *
 * @note    The service owns its internal fault state and must be initialized
 *          before the failsafe APIs are used.
 * @note    Fault transitions update the fault state before signaling the
 *          corresponding indication through the lighting service.
 */

#ifndef FAULT_FAULT_SERVICE_H_
#define FAULT_FAULT_SERVICE_H_

#include "stm32f4xx_hal.h"

/* ------------------ Public Fault APIs ------------------ */

/**
 * @brief Initialize the fault service.
 * @details Resets the service to normal operation and marks its singleton
 *          instance as ready for use.
 * @retval HAL_OK      Fault service initialized successfully.
 */
HAL_StatusTypeDef F_Init(void);

/**
 * @brief Enter failsafe operation.
 * @details Changes the fault state to failsafe and requests the corresponding
 *          lighting indication.
 * @retval HAL_OK      Failsafe state entered and lighting request accepted.
 * @retval HAL_ERROR   The service is uninitialized or system-fault state is
 *                     already active.
 * @retval HAL_TIMEOUT The lighting service remained busy until the retry
 *                     timeout expired.
 * @note    A system fault has priority and cannot be replaced by failsafe.
 */
HAL_StatusTypeDef F_EnterFailsafe(void);

/**
 * @brief Exit failsafe operation.
 * @details Returns the fault state to normal operation and requests the idle
 *          lighting indication.
 * @retval HAL_OK      Normal state restored and lighting request accepted.
 * @retval HAL_ERROR   The service is uninitialized or system-fault state is
 *                     already active.
 * @retval HAL_TIMEOUT The lighting service remained busy until the retry
 *                     timeout expired.
 * @note    This API cannot clear a system fault.
 */
HAL_StatusTypeDef F_ExitFailsafe(void);

/**
 * @brief Enter the latched system-fault state.
 * @details Records the system fault and requests the system-fault lighting
 *          indication. Once entered, the state cannot be cleared through the
 *          public fault-service APIs.
 * @note    This API has no status return value; lighting failures do not
 *          prevent the system-fault state from being recorded.
 */
void F_EnterSystemfault(void);

/* ----------------------------------------------------------- */

#endif /* FAULT_FAULT_SERVICE_H_ */

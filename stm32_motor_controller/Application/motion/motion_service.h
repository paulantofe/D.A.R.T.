/**
 * @file    motion_service.h
 * @brief   Public API for the rover motion service.
 * @details Stores the requested linear speed and movement direction, then
 *          converts them into safe left and right wheel targets for the RMC
 *          motor drivers.
 *
 * @note    The service owns its internal motor-driver instances and must be
 *          initialized before any other motion API is called.
 * @note    Motion processing is non-blocking. Call @ref M_Process from the
 *          application super loop after the service requests a control update.
 */

#ifndef MOTION_MOTION_SERVICE_H_
#define MOTION_MOTION_SERVICE_H_

#include "stm32f4xx_hal.h"

/**
 * @brief Requested rover movement direction.
 */
typedef enum {
	M_DIRECTION_STOP = 0,          /**< Stop both wheels. */
	M_DIRECTION_FORWARD,           /**< Move forward. */
	M_DIRECTION_FORWARD_LEFT,      /**< Move forward while curving left. */
	M_DIRECTION_LEFT,              /**< Spin left in place. */
	M_DIRECTION_BACKWARD_LEFT,     /**< Move backward while curving left. */
	M_DIRECTION_BACKWARD,          /**< Move backward. */
	M_DIRECTION_BACKWARD_RIGHT,    /**< Move backward while curving right. */
	M_DIRECTION_RIGHT,             /**< Spin right in place. */
	M_DIRECTION_FORWARD_RIGHT      /**< Move forward while curving right. */
} M_Direction_t;

/* ------------------ Public Motion APIs ------------------ */

/**
 * @brief Initialize the motion service.
 * @details Configures the motor GPIOs, PWM and encoder timers, initializes
 *          both RMC motor-driver instances, and resets the requested motion
 *          state.
 * @retval HAL_OK      Motion service initialized successfully.
 * @retval HAL_ERROR   Hardware configuration or motor-driver initialization
 *                     failed.
 */
HAL_StatusTypeDef M_Init(void);

/**
 * @brief Set the requested linear speed magnitude.
 * @param[in] speed Requested speed in millimeters per second.
 * @retval HAL_OK      Speed accepted and stored.
 * @retval HAL_ERROR   Service is uninitialized or speed is outside the
 *                     supported range.
 * @note    The direction sign is selected by @ref M_SetDirection; this value
 *          is a non-negative speed magnitude.
 */
HAL_StatusTypeDef M_SetLinearSpeed(float speed);

/**
 * @brief Set the requested rover direction.
 * @param[in] direction Direction command to store.
 * @retval HAL_OK      Direction accepted and stored.
 * @retval HAL_ERROR   Service is uninitialized or direction is invalid.
 * @note    The new direction is applied during the next @ref M_Process call.
 */
HAL_StatusTypeDef M_SetDirection(M_Direction_t direction);

/**
 * @brief Request one motion-control update.
 * @details Sets the pending-process flag consumed by @ref M_Process.
 * @note    Intended for the TIM6 period-elapsed callback and performs no
 *          blocking operations.
 */
void M_RequestMotionProcess(void);

/**
 * @brief Process one pending motion-control update.
 * @details Converts the requested direction and speed into left and right
 *          wheel targets, applies wheel-speed limits, handles large reversals,
 *          and updates both RMC motor controllers.
 * @retval HAL_OK      No update was pending or the update completed.
 * @retval HAL_ERROR   Service is uninitialized or a motion/RMC operation
 *                     failed.
 * @note    Call this API from the application super loop, not from an ISR.
 */
HAL_StatusTypeDef M_Process(void);

/* -------------------------------------------------------- */

#endif /* MOTION_MOTION_SERVICE_H_ */

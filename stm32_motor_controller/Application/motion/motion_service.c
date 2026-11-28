/**
 * @file    motion_service.c
 * @brief   Rover motion service implementation.
 * @details Coordinates the requested rover motion with the two RMC motor
 *          drivers and provides the periodic control-loop boundary.
 */

#include "motion_service.h"
#include "system_state_service.h"
#include "stm32f4xx_hal_hardware_config.h"
#include "board_peripherals.h"
#include "rmc_motor_driver.h"
#include <stdbool.h>

/* ------------------ Static Service Instances ------------------ */

static RMC_Handle_t hleft_motor;              /**< Internal left-wheel RMC instance. */
static RMC_Handle_t hright_motor;             /**< Internal right-wheel RMC instance. */
static bool is_initialized;                   /**< True after successful initialization. */
static float requested_linear_speed;          /**< Requested non-negative speed magnitude. */
static M_Direction_t requested_direction;     /**< Requested rover direction. */
static volatile bool motion_process_pending;  /**< True when a control update is pending. */

#define M_DIAGONAL_TURN_RATIO          0.5f   /**< Turn component for diagonal commands. */
#define M_MAX_LINEAR_SPEED_MM_S        250.0f /**< Maximum requested wheel speed. */
#define M_MAX_DIFFERENCE_MM_S          40.0f  /**< Difference above which reversal is delayed. */
#define M_REST_SPEED_THRESHOLD_MM_S    5.0f   /**< Maximum measured speed considered stopped. */

/**
 * @brief Check whether a direction enum value is supported.
 * @param[in] direction Direction value to validate.
 * @return true when the direction is valid, otherwise false.
 */
static bool is_valid_direction(M_Direction_t direction) {
	return direction >= 0 && direction <= 8;
}

/**
 * @brief Calculate the absolute difference between two speeds.
 * @param[in] speed1 First speed value.
 * @param[in] speed2 Second speed value.
 * @return Absolute speed difference.
 */
static float calculate_difference(float speed1, float speed2) {
	float difference = speed1 - speed2;

	return difference < 0.0f ? -difference : difference;
}

/**
 * @brief Detect a large reversal between measured and requested wheel speed.
 * @param[in] current_speed Current measured wheel speed.
 * @param[in] target_speed Requested wheel target speed.
 * @return true when the signs differ and the speed change exceeds the limit.
 */
static bool M_HasLargeReversal(float current_speed, float target_speed) {
	bool opposite_signs;

	opposite_signs = current_speed != 0.0f
			&& target_speed != 0.0f
			&& ((current_speed < 0.0f) != (target_speed < 0.0f));

	return opposite_signs
			&& calculate_difference(current_speed, target_speed)
				> M_MAX_DIFFERENCE_MM_S;
}

/**
 * @brief Command both motor targets to zero after an internal failure.
 */
static void M_ResetMotion(void) {
	(void)RMC_SetTargetSpeed(&hleft_motor, 0.0f);
	(void)RMC_SetTargetSpeed(&hright_motor, 0.0f);
	requested_direction = M_DIRECTION_STOP;
}

/**
 * @brief Convert the requested direction into linear and turning components.
 * @param[out] linear_speed Linear component in millimeters per second.
 * @param[out] turn Turning component in millimeters per second.
 * @retval HAL_OK Components calculated successfully.
 * @retval HAL_ERROR An output pointer is null or the stored direction is invalid.
 */
static HAL_StatusTypeDef M_GetMotionComponents(
		float *linear_speed,
		float *turn) {
	if (linear_speed == NULL || turn == NULL) {
		return HAL_ERROR;
	}

	switch (requested_direction) {
	case M_DIRECTION_STOP:
		*linear_speed = 0.0f;
		*turn = 0.0f;
		break;

	case M_DIRECTION_FORWARD:
		*linear_speed = requested_linear_speed;
		*turn = 0.0f;
		break;

	case M_DIRECTION_BACKWARD:
		*linear_speed = -requested_linear_speed;
		*turn = 0.0f;
		break;

	case M_DIRECTION_LEFT:
		*linear_speed = 0.0f;
		*turn = requested_linear_speed;
		break;

	case M_DIRECTION_RIGHT:
		*linear_speed = 0.0f;
		*turn = -requested_linear_speed;
		break;

	case M_DIRECTION_FORWARD_LEFT:
		*linear_speed = requested_linear_speed;
		*turn = requested_linear_speed * M_DIAGONAL_TURN_RATIO;
		break;

	case M_DIRECTION_FORWARD_RIGHT:
		*linear_speed = requested_linear_speed;
		*turn = -requested_linear_speed * M_DIAGONAL_TURN_RATIO;
		break;

	case M_DIRECTION_BACKWARD_LEFT:
		*linear_speed = -requested_linear_speed;
		*turn = requested_linear_speed * M_DIAGONAL_TURN_RATIO;
		break;

	case M_DIRECTION_BACKWARD_RIGHT:
		*linear_speed = -requested_linear_speed;
		*turn = -requested_linear_speed * M_DIAGONAL_TURN_RATIO;
		break;

	default:
		*linear_speed = 0.0f;
		*turn = 0.0f;
		return HAL_ERROR;
	}

	return HAL_OK;
}

/**
 * @brief Scale wheel targets proportionally to the maximum wheel-speed limit.
 * @param[in,out] left_speed Left-wheel target speed.
 * @param[in,out] right_speed Right-wheel target speed.
 */
static void M_LimitWheelSpeeds(float *left_speed, float *right_speed) {
	float left_abs_speed;
	float right_abs_speed;
	float max_speed;
	float scale;

	if (left_speed == NULL || right_speed == NULL) {
		return;
	}

	left_abs_speed = *left_speed < 0.0f ? -*left_speed : *left_speed;
	right_abs_speed = *right_speed < 0.0f ? -*right_speed : *right_speed;
	max_speed = left_abs_speed > right_abs_speed
			? left_abs_speed
			: right_abs_speed;

	if (max_speed <= M_MAX_LINEAR_SPEED_MM_S) {
		return;
	}

	scale = M_MAX_LINEAR_SPEED_MM_S / max_speed;
	*left_speed *= scale;
	*right_speed *= scale;
}

/**
 * @brief Determine whether either wheel requires a large reversal.
 * @param[in] left_speed Requested left-wheel target speed.
 * @param[in] right_speed Requested right-wheel target speed.
 * @return true when a reversal is detected or speed feedback is unavailable.
 */
static bool M_ReversalRequired(float left_speed, float right_speed) {
	float current_left_speed;
	float current_right_speed;

	if (RMC_GetSpeed(&hleft_motor, &current_left_speed) != HAL_OK
			|| RMC_GetSpeed(&hright_motor, &current_right_speed) != HAL_OK) {
		return true;
	}

	return M_HasLargeReversal(current_left_speed, left_speed)
			|| M_HasLargeReversal(current_right_speed, right_speed);
}

/**
 * @brief Check whether both measured wheel speeds are within the rest threshold.
 * @return true when both motors are considered stopped.
 */
static bool M_MotorsAtRest(void) {
	float left_speed, right_speed;

	if (RMC_GetSpeed(&hleft_motor, &left_speed) != HAL_OK
			|| RMC_GetSpeed(&hright_motor, &right_speed) != HAL_OK) {
		return false;
	}

	return calculate_difference(left_speed, 0.0f)
			<= M_REST_SPEED_THRESHOLD_MM_S
			&& calculate_difference(right_speed, 0.0f)
			<= M_REST_SPEED_THRESHOLD_MM_S;
}

/* ---------------------------------------------------------------- */


/* ------------------ Public APIs Implementation ------------------ */

HAL_StatusTypeDef M_Init(void) {
	HAL_StatusTypeDef status;

	is_initialized = false;

	(void)L298N_INPins_Config();

	status = TIM6_Config();
	if (status != HAL_OK) {
		return status;
	}

	status = TIM3_Config();
	if (status != HAL_OK) {
		return status;
	}

	status = TIM2_Config();
	if (status != HAL_OK) {
		return status;
	}

	status = TIM5_Config();
	if (status != HAL_OK) {
		return status;
	}

	hleft_motor.config.motor_rpm = 90U;
	hleft_motor.config.htim_pwm = &htim3;
	hleft_motor.config.pwm_channel = TIM_CHANNEL_1;
	hleft_motor.config.invert_dir = RMC_INVERT_DIR_DISABLE;
	hleft_motor.config.htim_enc = &htim2;
	hleft_motor.config.wheel_circum_mm = 204U;
	hleft_motor.config.ticks_per_wheel_rev = 2475U;
	hleft_motor.config.delta_t_ms = 10U;
	hleft_motor.config.in1_port = GPIOD;
	hleft_motor.config.in1_pin = GPIO_PIN_0;
	hleft_motor.config.in2_port = GPIOD;
	hleft_motor.config.in2_pin = GPIO_PIN_1;
	hleft_motor.config.kp = 0.0f;
	hleft_motor.config.ki = 0.0f;
	hleft_motor.config.kd = 0.0f;
	if (RMC_Init(&hleft_motor) != HAL_OK) {
		SS_RequestState(SS_State_Systemfault);
		return HAL_ERROR;
	}

	hright_motor.config.motor_rpm = 90U;
	hright_motor.config.htim_pwm = &htim3;
	hright_motor.config.pwm_channel = TIM_CHANNEL_2;
	hright_motor.config.invert_dir = RMC_INVERT_DIR_DISABLE;
	hright_motor.config.htim_enc = &htim5;
	hright_motor.config.wheel_circum_mm = 204U;
	hright_motor.config.ticks_per_wheel_rev = 2475U;
	hright_motor.config.delta_t_ms = 10U;
	hright_motor.config.in1_port = GPIOD;
	hright_motor.config.in1_pin = GPIO_PIN_2;
	hright_motor.config.in2_port = GPIOD;
	hright_motor.config.in2_pin = GPIO_PIN_3;
	hright_motor.config.kp = 0.0f;
	hright_motor.config.ki = 0.0f;
	hright_motor.config.kd = 0.0f;
	if (RMC_Init(&hright_motor) != HAL_OK) {
		SS_RequestState(SS_State_Systemfault);
		return HAL_ERROR;
	}

	requested_linear_speed = 0.0f;
	requested_direction = M_DIRECTION_STOP;
	motion_process_pending = false;

	status = HAL_TIM_Base_Start_IT(&htim6);
	if (status != HAL_OK) {
		SS_RequestState(SS_State_Systemfault);
		return HAL_ERROR;
	}

	is_initialized = true;
	return HAL_OK;
}

HAL_StatusTypeDef M_SetLinearSpeed(float speed) {
	if (!is_initialized || speed < 0.0f || speed > M_MAX_LINEAR_SPEED_MM_S) {
		return HAL_ERROR;
	}

	requested_linear_speed = speed;

	return HAL_OK;
}

HAL_StatusTypeDef M_SetDirection(M_Direction_t direction) {
	if (!is_initialized || !is_valid_direction(direction)) {
		return HAL_ERROR;
	}

	requested_direction = direction;

	return HAL_OK;
}

void M_RequestMotionProcess(void) {
	if (!is_initialized) {
		return;
	}

	motion_process_pending = true;
}

HAL_StatusTypeDef M_Process(void) {
	float linear_speed;
	float turn_speed;
	float left_speed;
	float right_speed;
	HAL_StatusTypeDef status;

	if (!is_initialized) {
		return HAL_ERROR;
	}

	if (!motion_process_pending) {
		return HAL_OK;
	}

	motion_process_pending = false;

	status = M_GetMotionComponents(&linear_speed, &turn_speed);
	if (status != HAL_OK) {
		M_ResetMotion();
		return status;
	}

	left_speed = linear_speed - turn_speed;
	right_speed = linear_speed + turn_speed;
	M_LimitWheelSpeeds(&left_speed, &right_speed);

	if (M_ReversalRequired(left_speed, right_speed) &&
			!M_MotorsAtRest()) {
		if (RMC_SetTargetSpeed(&hleft_motor, 0.0f) != HAL_OK
				|| RMC_SetTargetSpeed(&hright_motor, 0.0f) != HAL_OK) {
			M_ResetMotion();
			return HAL_ERROR;
		}

		RMC_TriggerUpdate(&hleft_motor);
		RMC_TriggerUpdate(&hright_motor);

		if (RMC_Update(&hleft_motor) != HAL_OK
				|| RMC_Update(&hright_motor) != HAL_OK) {
			M_ResetMotion();
			return HAL_ERROR;
		}

		return HAL_OK;
	}

    if (RMC_SetTargetSpeed(&hleft_motor, left_speed) != HAL_OK
			|| RMC_SetTargetSpeed(&hright_motor, right_speed) != HAL_OK) {
		M_ResetMotion();
		return HAL_ERROR;
	}

	RMC_TriggerUpdate(&hleft_motor);
	RMC_TriggerUpdate(&hright_motor);

	if (RMC_Update(&hleft_motor) != HAL_OK
			|| RMC_Update(&hright_motor) != HAL_OK) {
		M_ResetMotion();
		return HAL_ERROR;
	}

	return HAL_OK;
}

/* ---------------------------------------------------------------- */

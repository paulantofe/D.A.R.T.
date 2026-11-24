/**
 * @file  rmc_motor_driver.h
 * @brief Rover Motor Controller driver header file providing APIs and macros to
 *        control and configure DC motors and encoders using HAL_APIs.
 *
 * @note  This driver is developed and tested with the following hardware:
 *           - L298N Motor Driver
 *           - JGB37-520 DC Motors
 */

#ifndef RMC_MOTOR_DRIVER_H
#define RMC_MOTOR_DRIVER_H

#include "stm32f4xx_hal.h"
#include <stdint.h>
#include <stdbool.h>

/* ------------------ General Macros Definition ------------------ */

/**
 * @defgroup RMC_INVERT_DIR         Invert Direction
 * @{
 */
#define RMC_INVERT_DIR_DISABLE       0U
#define RMC_INVERT_DIR_ENABLE        1U
/** @} */

/**
 * @defgroup RMC_HARD_BRAKE          Hard Brake
 * @{
 */
#define RMC_HARD_BRAKE_DISABLE       0U      // active braking
#define RMC_HARD_BRAKE_ENABLE        1U      // coasting
/** @} */

/* --------------------------------------------------------------- */


/* ------------------ Motor Controller Structures Definition ------------------ */

/* Motor Directions */
typedef enum {
	RMC_DIR_COAST = 0,                 /**< IN1=0, IN2=0 */
	RMC_DIR_FORWARD,                   /**< Forward rotation */
	RMC_DIR_REVERSE,                   /**< Reverse rotation */
	RMC_DIR_BRAKE                      /**< IN1=1, IN2=1 (active short brake) */
} RMC_Dir_t;

/* Configuration Structure */
typedef struct {
	/* Motor & PWM Configuration */
	uint16_t motor_rpm;                /**< DC motor nominal RPM */
	TIM_HandleTypeDef *htim_pwm;       /**< Pointer to TIM handle in PWM mode */
	uint32_t pwm_channel;              /**< PWM channel @ref TIM_Channel */
	uint8_t invert_dir;                /**< Invert direction @ref RMC_INVERT_DIR */

	/* Encoder Configuration */
	TIM_HandleTypeDef *htim_enc;       /**< Pointer to the TIM handle in Encoder mode */
	uint16_t wheel_circum_mm;          /**< Wheel circumference in millimeters */
	uint32_t ticks_per_wheel_rev;      /**< Number of encoder ticks per full wheel revolution */
	uint8_t delta_t_ms;                /**< Sampling period in milliseconds */

	/* H-Bridge Directions Pins */
	GPIO_TypeDef *in1_port;            /**< Pointer to the IN1 GPIO port (e.g. GPIOA) */
	uint16_t in1_pin;                  /**< IN1 GPIO pin mask (e.g. GPIO_PIN_0) */
	GPIO_TypeDef *in2_port;            /**< Pointer to the IN2 GPIO port (e.g. GPIOA) */
	uint16_t in2_pin;                  /**< IN2 GPIO pin mask (e.g. GPIO_PIN_1) */

	/* PID Configuration */
	float kp;                          /**< Proportional gain */
	float ki;                          /**< Integral gain */
    float kd;                          /**< Derivative gain */
} RMC_Config_t;

/* Handle Structure */
typedef struct {
	RMC_Config_t config;      /**< Static hardware and PID configuration provided by user */

	/* Precomputed Constants (set in RMC_Init) */
	float mm_per_tick;        /**< Wheel_circum_mm / ticks_per_wheel_rev */
	float dt_sec;             /**< delta_t_ms / 1000.0f */
	uint32_t pwm_max_duty;    /**< Timer ARR value (100% duty cycle) */

	/* Runtime State */
	uint32_t prev_enc_cnt;    /**< Previous encoder timer counter value */
	float speed_mm_s;         /**< Measured linear speed [mm/s] */
	float target_speed_mm_s;  /**< Target linear speed [mm/s] */
	RMC_Dir_t direction;      /**< Direction of the motor rotation */
	float pid_integral;       /**< Accumulated integral term */
	float pid_prev_error;     /**< Previous error for derivative term */
	bool is_initialized;      /**< Initialization guard flag */
} RMC_Handle_t;

/* ---------------------------------------------------------------------------- */


/* ------------------ Public APIs ------------------ */

/**
 * @brief Initialize the rover motors.
 * @param[in,out] rmc   pointer to RMC handle structure with
 *                      desired configuration.
 * @retval HAL_OK       Initialization and configuration succeeded.
 * @retval HAL_ERROR    Invalid configuration parameter.
 */
HAL_StatusTypeDef RMC_Init(RMC_Handle_t *hrmc);

/**
 * @brief Set the motor output PWM.
 * @param[in] hrmc            pointer to RMC handle.
 * @param[in] duty_percent    duty cycle percent [0, 100].
 * @retval HAL_OK       Provided PWM set.
 * @retval HAL_ERROR    Invalid pointers or handle not initialized.
 * @note Invalid percents are clamped to [0, 100] (e.g. 120 -> 100,
 *       -120 -> 0)
 */
HAL_StatusTypeDef RMC_SetRawPWM(RMC_Handle_t *hrmc, float duty_percent);

/**
 * @brief Set the motor rotation direction.
 * @param[in] hrmc      pointer to RMC handle.
 * @param[in] direction rotation direction.
 * @retval HAL_OK       Provided rotation set.
 * @retval HAL_ERROR    Invalid pointers or handle not initialized.
 */
HAL_StatusTypeDef RMC_SetDirection(RMC_Handle_t *hrmc, const RMC_Dir_t direction);

/**
 * @brief Stop the motor.
 * @param[in] hrmc       pointer to RMC handle.
 * @param[in] hard_brake RMC_HARD_BRAKE_DISABLE / _ENABLE.
 * @retval HAL_OK    Motor stopped.
 * @retval HAL_ERROR Invalid pointers or handle not initialized.
 * @note Hard brake enable means active braking.
 *       Hard brake disable means coasting.
 */
HAL_StatusTypeDef RMC_Stop(RMC_Handle_t *hrmc, const uint8_t hard_brake);

/**
 * @brief Get the current motor speed.
 * @param[in] hrmc       pointer to RMC handle.
 * @param[out] speed    current speed of the motor.
 * @retval HAL_OK       Current speed provided.
 * @retval HAL_ERROR    Invalid pointers or handle not initialized.
 */
HAL_StatusTypeDef RMC_GetSpeed(const RMC_Handle_t *hrmc, float *speed);

/**
 * @brief Get the linear speed produced by two motors.
 * @param[in] hrmc1     pointer to motor1 RMC handle.
 * @param[in] hrmc2     pointer to motor2 RMC handle.
 * @param[out] speed    current linear speed.
 * @retval HAL_OK       Current linear speed provided.
 * @retval HAL_ERROR    Invalid pointers or handle not initialized.
 */
HAL_StatusTypeDef RMC_GetLinearSpeed(const RMC_Handle_t *hrmc1, const RMC_Handle_t *hrmc2, float *speed);

/* ------------------------------------------------- */

#endif /* RMC_MOTOR_DRIVER_H */

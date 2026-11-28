#include "motion_service.h"
#include "system_state_service.h"
#include "stm32f4xx_hal_hardware_config.h"
#include "rmc_motor_driver.h"
#include <stdbool.h>

/* ------------------ Static Service Instances ------------------ */

static RMC_Handle_t hleft_motor;
static RMC_Handle_t hright_motor;
static bool is_initialized;

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

	is_initialized = true;
	return HAL_OK;
}

/* ---------------------------------------------------------------- */

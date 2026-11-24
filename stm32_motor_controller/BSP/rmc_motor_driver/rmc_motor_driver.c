/**
 * @file  rmc_motor_driver.c
 * @brief Rover Motor Controller driver source file providing APIs implementations for
 *        controlling and configuring DC motors and encoders.
 */

#include <rmc_motor_driver/rmc_motor_driver.h>

/* ------------------ Static Helper Functions ------------------ */

static inline HAL_StatusTypeDef RMC_CheckPointers(const RMC_Handle_t *hrmc) {
	if (hrmc == NULL || hrmc->config.htim_enc == NULL ||
		hrmc->config.in1_port == NULL || hrmc->config.in2_port == NULL ||
		hrmc->config.htim_pwm == NULL) {
		return HAL_ERROR;
	}
	return HAL_OK;
}

static inline HAL_StatusTypeDef RMC_CheckConfig(const RMC_Handle_t *hrmc) {
	if (hrmc->config.ticks_per_wheel_rev == 0U ||
		hrmc->config.delta_t_ms == 0U ||
		hrmc->config.wheel_circum_mm == 0U ||
		__HAL_TIM_GET_AUTORELOAD(hrmc->config.htim_pwm) == 0U) {
		return HAL_ERROR;
	}
	return HAL_OK;
}

/* ------------------------------------------------------------- */


/* ------------------ Public APIs Implementation ------------------ */

HAL_StatusTypeDef RMC_Init(RMC_Handle_t *hrmc) {
	HAL_StatusTypeDef status = RMC_CheckPointers(hrmc);
	if (status != HAL_OK) {
		return status;
	}

	status = RMC_CheckConfig(hrmc);
	if (status != HAL_OK) {
		return status;
	}

	hrmc->is_initialized = false;

	__HAL_TIM_SET_COMPARE(hrmc->config.htim_pwm, hrmc->config.pwm_channel, 0);
	HAL_GPIO_WritePin(hrmc->config.in1_port, hrmc->config.in1_pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(hrmc->config.in2_port, hrmc->config.in2_pin, GPIO_PIN_RESET);

	hrmc->mm_per_tick = (float)hrmc->config.wheel_circum_mm / (float)hrmc->config.ticks_per_wheel_rev;
	hrmc->dt_sec = (float)hrmc->config.delta_t_ms / 1000.0f;
	hrmc->pwm_max_duty = __HAL_TIM_GET_AUTORELOAD(hrmc->config.htim_pwm);

	hrmc->speed_mm_s = 0.0f;
	hrmc->target_speed_mm_s = 0.0f;
	hrmc->pid_integral = 0.0f;
	hrmc->pid_prev_error = 0.0f;
	hrmc->prev_enc_cnt = 0U;
	__HAL_TIM_SET_COUNTER(hrmc->config.htim_enc, 0U);

	status = HAL_TIM_PWM_Start(hrmc->config.htim_pwm, hrmc->config.pwm_channel);
	if (status != HAL_OK) {
		return status;
	}

	status = HAL_TIM_Encoder_Start(hrmc->config.htim_enc, TIM_CHANNEL_ALL);
	if (status != HAL_OK) {
		return status;
	}

	hrmc->is_initialized = true;
	return HAL_OK;
}

HAL_StatusTypeDef RMC_SetRawPWM(RMC_Handle_t *hrmc, float duty_percent) {
	if (hrmc == (void*)0 || !hrmc->is_initialized) {
		return HAL_ERROR;
	}

	if (duty_percent > 100.0f)    { duty_percent = 100.0f; }
	else if (duty_percent < 0.0f) { duty_percent = 0.0f;   }

	uint32_t ccr = (uint32_t)((duty_percent / 100.0f) * (float)hrmc->pwm_max_duty);

	__HAL_TIM_SET_COMPARE(hrmc->config.htim_pwm, hrmc->config.pwm_channel, ccr);

	return HAL_OK;
}

HAL_StatusTypeDef RMC_GetSpeed(const RMC_Handle_t *hrmc, float *speed) {
	if (hrmc == (void*)0 || speed == (void*)0 || !hrmc->is_initialized) {
		return HAL_ERROR;
	}

	*speed = hrmc->speed_mm_s;

	return HAL_OK;
}

/* ---------------------------------------------------------------- */

/**
 * @file  rmc_motor_driver.c
 * @brief Rover Motor Controller driver source file providing APIs implementations for
 *        controlling and configuring DC motors and encoders.
 */

#include "rmc_motor_driver/rmc_motor_driver.h"

/* ------------------ Static Helper Functions ------------------ */

static inline HAL_StatusTypeDef RMC_CheckPointers(const RMC_Handle_t *hrmc) {
	if (hrmc == (void*)0 || hrmc->config.htim_enc == (void*)0 ||
		hrmc->config.in1_port == (void*)0 || hrmc->config.in2_port == (void*)0 ||
		hrmc->config.htim_pwm == (void*)0) {
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
	hrmc->direction = RMC_DIR_COAST;
	hrmc->pid_integral = 0.0f;
	hrmc->pid_prev_error = 0.0f;
	hrmc->prev_enc_cnt = 0U;
	hrmc->update_pending = false;
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

HAL_StatusTypeDef RMC_Update(RMC_Handle_t *hrmc) {
	if (hrmc == (void*)0 || !hrmc->is_initialized) {
		return HAL_ERROR;
	}

	if (!hrmc->update_pending) { return HAL_OK; }

	hrmc->update_pending = false;

	uint32_t current_enc_cnt = __HAL_TIM_GET_COUNTER(hrmc->config.htim_enc);
	int32_t delta_ticks      = (int32_t)(current_enc_cnt - hrmc->prev_enc_cnt);
	hrmc->prev_enc_cnt       = current_enc_cnt;

	float distance_mm = (float)delta_ticks * hrmc->mm_per_tick;
	hrmc->speed_mm_s  = distance_mm / hrmc->dt_sec;

	if (hrmc->target_speed_mm_s == 0.0f) {
		hrmc->pid_integral   = 0.0f;
		hrmc->pid_prev_error = 0.0f;
		RMC_SetDirection(hrmc, RMC_DIR_COAST);
		return RMC_SetRawPWM(hrmc, 0.0f);
	}

	float error = hrmc->target_speed_mm_s - hrmc->speed_mm_s;

    float P = hrmc->config.kp * error;

    hrmc->pid_integral += hrmc->config.ki * error * hrmc->dt_sec;
    if (hrmc->pid_integral > 100.0f)       { hrmc->pid_integral =  100.0f; }
    else if (hrmc->pid_integral < -100.0f) { hrmc->pid_integral = -100.0f; }

    float D = hrmc->config.kd * ((error - hrmc->pid_prev_error) / hrmc->dt_sec);
    hrmc->pid_prev_error = error;

	float output = P + hrmc->pid_integral + D;
	if (output > 100.0f)       { output =  100.0f; }
	else if (output < -100.0f) { output = -100.0f; }

	if (hrmc->target_speed_mm_s > 0.0f) {
		if (output > 0.0f) {
			RMC_SetDirection(hrmc, RMC_DIR_FORWARD);
			return RMC_SetRawPWM(hrmc, output);
		} else {
			RMC_SetDirection(hrmc, RMC_DIR_COAST);
			return RMC_SetRawPWM(hrmc, 0.0f);
		}
	} else {
		if (output < 0.0f) {
			RMC_SetDirection(hrmc, RMC_DIR_REVERSE);
			return RMC_SetRawPWM(hrmc, -output);
		} else {
			RMC_SetDirection(hrmc, RMC_DIR_COAST);
			return RMC_SetRawPWM(hrmc, 0.0f);
		}
	}

	return HAL_OK;
}

void RMC_TriggerUpdate(RMC_Handle_t *hrmc) {
	if (hrmc != (void*)0 && hrmc->is_initialized) {
		hrmc->update_pending = true;
	}
}

HAL_StatusTypeDef RMC_SetTargetSpeed(RMC_Handle_t *hrmc, const float target_speed) {
	if (hrmc != (void*)0 && hrmc->is_initialized) {
		hrmc->target_speed_mm_s = target_speed;

		return HAL_OK;
	}

	return HAL_ERROR;
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

HAL_StatusTypeDef RMC_SetDirection(RMC_Handle_t *hrmc, const RMC_Dir_t direction) {
	if (hrmc == (void*)0 || !hrmc->is_initialized) {
		return HAL_ERROR;
	}

	if (direction == hrmc->direction) {
		return HAL_OK;
	}

	RMC_Dir_t phys_dir = direction;
	if (hrmc->config.invert_dir == RMC_INVERT_DIR_ENABLE) {
		if (direction == RMC_DIR_FORWARD) {
			phys_dir = RMC_DIR_REVERSE;
		} else if (direction == RMC_DIR_REVERSE) {
			phys_dir = RMC_DIR_FORWARD;
		}
	}

	switch (phys_dir) {
	    case RMC_DIR_COAST:
	    	    HAL_GPIO_WritePin(hrmc->config.in1_port, hrmc->config.in1_pin, GPIO_PIN_RESET);
	    	    HAL_GPIO_WritePin(hrmc->config.in2_port, hrmc->config.in2_pin, GPIO_PIN_RESET);
	    	    break;

	    case RMC_DIR_FORWARD:
	    	    HAL_GPIO_WritePin(hrmc->config.in1_port, hrmc->config.in1_pin, GPIO_PIN_SET);
	    		HAL_GPIO_WritePin(hrmc->config.in2_port, hrmc->config.in2_pin, GPIO_PIN_RESET);
	    		break;

	    case RMC_DIR_REVERSE:
	    	    HAL_GPIO_WritePin(hrmc->config.in1_port, hrmc->config.in1_pin, GPIO_PIN_RESET);
	    	    HAL_GPIO_WritePin(hrmc->config.in2_port, hrmc->config.in2_pin, GPIO_PIN_SET);
	    	    break;

	    case RMC_DIR_BRAKE:
	    	    	HAL_GPIO_WritePin(hrmc->config.in1_port, hrmc->config.in1_pin, GPIO_PIN_SET);
	    	    HAL_GPIO_WritePin(hrmc->config.in2_port, hrmc->config.in2_pin, GPIO_PIN_SET);
	    	    	break;

	    default:
	    	    return HAL_ERROR;
	}

    hrmc->direction = direction;

	return HAL_OK;
}

HAL_StatusTypeDef RMC_Stop(RMC_Handle_t *hrmc, const uint8_t hard_brake) {
	if (hrmc == (void*)0 || !hrmc->is_initialized) {
		return HAL_ERROR;
	}

	if (hard_brake != RMC_HARD_BRAKE_ENABLE && hard_brake != RMC_HARD_BRAKE_DISABLE) {
		return HAL_ERROR;
	}

	hrmc->target_speed_mm_s = 0.0f;
	hrmc->pid_integral = 0.0f;
	hrmc->pid_prev_error = 0.0f;

	if (hard_brake == RMC_HARD_BRAKE_ENABLE) {
		if (RMC_SetDirection(hrmc, RMC_DIR_BRAKE) != HAL_OK) {
			return HAL_ERROR;
		}
		return RMC_SetRawPWM(hrmc, 100.0f);
	}

	if (RMC_SetDirection(hrmc, RMC_DIR_COAST) != HAL_OK) {
		return HAL_ERROR;
	}
	return RMC_SetRawPWM(hrmc, 0.0f);
}

HAL_StatusTypeDef RMC_GetSpeed(const RMC_Handle_t *hrmc, float *speed) {
	if (hrmc == (void*)0 || speed == (void*)0 || !hrmc->is_initialized) {
		return HAL_ERROR;
	}

	*speed = hrmc->speed_mm_s;

	return HAL_OK;
}

HAL_StatusTypeDef RMC_GetLinearSpeed(const RMC_Handle_t *hrmc1, const RMC_Handle_t *hrmc2, float *speed) {
	if (hrmc1 == (void*)0  || !hrmc1->is_initialized ||
		hrmc2 == (void*)0  || !hrmc2->is_initialized ||
		speed == (void*)0) {
		return HAL_ERROR;
	}

	*speed = ((hrmc1->speed_mm_s + hrmc2->speed_mm_s) / 2.0f);

	return HAL_OK;
}

/* ---------------------------------------------------------------- */

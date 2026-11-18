/**
 * @file     rmc_motor_controller.c
 * @brief    Rover Motor Controller source file
 *
 * @details  This file implements the APIs needed to control the rover motors.
 */

#include "rmc_motor_controller.h"
#include "stm32f4xx_hal_hardware_config.h"

/* ------------------ Motor Structures Definition ------------------ */

typedef struct {
	/* Motor Input */
	TIM_HandleTypeDef *htim_enc;  /**< pointer to TIM peripheral in encoder mode */
	uint32_t enc_last_pos;        /**< last position of the encoder */

	/* Motor State */
	volatile float current_speed; /**< current speed in mps */
	float target_speed;           /**< target speed in mps */

	/* Motor Output */
	TIM_HandleTypeDef *htim_pwm;  /**< pointer to TIM peripheral in PWM mode */
	uint32_t pwm_channel;         /**< PWM Channel of the TIM */
} RMC_Motor_t;

static RMC_Motor_t left_motor;
static RMC_Motor_t right_motor;

/* ----------------------------------------------------------------- */


/* ------------------ Public Motor APIs Implementation ------------------ */

void RMC_Motor_Init() {
	TIM2_Config();
	TIM5_Config();
	TIM3_Config();

	left_motor.htim_enc = &htim2;
	HAL_TIM_Encoder_Start(left_motor.htim_enc, TIM_CHANNEL_ALL);
	left_motor.enc_last_pos = __HAL_TIM_GET_COUNTER(left_motor.htim_enc);
	left_motor.current_speed = 0.0f;
	left_motor.target_speed = 0.0f;
	left_motor.htim_pwm = &htim3;
	left_motor.pwm_channel = TIM_CHANNEL_1;
	HAL_TIM_PWM_Start(left_motor.htim_pwm, left_motor.pwm_channel);

	right_motor.htim_enc = &htim5;
	HAL_TIM_Encoder_Start(right_motor.htim_enc, TIM_CHANNEL_ALL);
	right_motor.enc_last_pos = __HAL_TIM_GET_COUNTER(right_motor.htim_enc);
	right_motor.current_speed = 0.0f;
	right_motor.target_speed = 0.0f;
	right_motor.htim_pwm = &htim3;
	right_motor.pwm_channel = TIM_CHANNEL_2;
	HAL_TIM_PWM_Start(right_motor.htim_pwm, right_motor.pwm_channel);
}

/* ---------------------------------------------------------------------- */

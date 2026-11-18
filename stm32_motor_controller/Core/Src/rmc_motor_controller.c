/**
 * @file     rmc_motor_controller.c
 * @brief    Rover Motor Controller source file
 *
 * @details  This file implements the APIs needed to control the rover motors.
 */

#include "rmc_motor_controller.h"
#include "stm32f4xx_hal_hardware_config.h"
#include "rdt_protocol.h"

/* ------------------ Motor Structures Definition ------------------ */

#define RMC_DELTA_T_S                0.01f
#define RMC_TICKS_PER_WHEEL_REV      2475.0f
#define RMC_WHEEL_CIRCUM_M           0.2042f
#define RMC_TICKS_TO_MPS             (RMC_WHEEL_CIRCUM_M / (RMC_TICKS_PER_WHEEL_REV * RMC_DELTA_T_S))

#define RMC_LEFT_MOTOR_IN1_PORT      GPIOD
#define RMC_LEFT_MOTOR_IN1_PIN       GPIO_PIN_0
#define RMC_LEFT_MOTOR_IN2_PORT      GPIOD
#define RMC_LEFT_MOTOR_IN2_PIN       GPIO_PIN_1

#define RMC_RIGHT_MOTOR_IN3_PORT     GPIOD
#define RMC_RIGHT_MOTOR_IN3_PIN      GPIO_PIN_2
#define RMC_RIGHT_MOTOR_IN4_PORT     GPIOD
#define RMC_RIGHT_MOTOR_IN4_PIN      GPIO_PIN_3

#define RMC_POWER_FULL               100.0f
#define RMC_POWER_TURN               60.0f
#define RMC_POWER_SPIN               50.0f

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

	/* Motor Direction (L298N IN pins) */
	GPIO_TypeDef *dir_port_1;      /**< INx Port */
    uint16_t dir_pin_1;            /**< INx Pin  */
	GPIO_TypeDef *dir_port_2;      /**< INx Port */
	uint16_t dir_pin_2;            /**< INx Pin  */

} RMC_Motor_t;

static RMC_Motor_t left_motor;
static RMC_Motor_t right_motor;

/* ----------------------------------------------------------------- */


/* ------------------ Static Helper Functions ------------------ */

/**
 * @brief     Set a motor's output power.
 * @param[in] motor     pointer to left_motor / right_motor RMC_Motor_t static variables.
 * @param[in] power_pct power percentage of the motor [-100.0, 100.0].
 * @details   Negative power means going backward, 0 power means coasting.
 */
static void RMC_Motor_SetPower(RMC_Motor_t *motor, float power_pct) {
	if (power_pct > 100.0f) { power_pct = 100.0f; }
	else if (power_pct < -100.0f) { power_pct = -100.0f; }

	if (power_pct > 0.0f) {
		HAL_GPIO_WritePin(motor->dir_port_1, motor->dir_pin_1, GPIO_PIN_SET);
		HAL_GPIO_WritePin(motor->dir_port_2, motor->dir_pin_2, GPIO_PIN_RESET);
	}
	else if (power_pct < 0.0f) {
		HAL_GPIO_WritePin(motor->dir_port_1, motor->dir_pin_1, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(motor->dir_port_2, motor->dir_pin_2, GPIO_PIN_SET);
		power_pct = -power_pct;
	} else {
		HAL_GPIO_WritePin(motor->dir_port_1, motor->dir_pin_1, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(motor->dir_port_2, motor->dir_pin_2, GPIO_PIN_RESET);
	}

	uint32_t arr = __HAL_TIM_GET_AUTORELOAD(motor->htim_pwm);
	uint32_t duty_cycle = (uint32_t) ((power_pct / 100.0f) * (float)arr);

	__HAL_TIM_SET_COMPARE(motor->htim_pwm, motor->pwm_channel, duty_cycle);
}

/* ------------------------------------------------------------- */


/* ------------------ Public Motor APIs Implementation ------------------ */

void RMC_Motor_Init(void) {
	TIM2_Config();
	TIM5_Config();
	TIM3_Config();
	L298N_INPins_Config();

	left_motor.htim_enc = &htim2;
	left_motor.dir_port_1 = RMC_LEFT_MOTOR_IN1_PORT;
	left_motor.dir_pin_1 = RMC_LEFT_MOTOR_IN1_PIN;
	left_motor.dir_port_2 = RMC_LEFT_MOTOR_IN2_PORT;
	left_motor.dir_pin_2 = RMC_LEFT_MOTOR_IN2_PIN;
	HAL_TIM_Encoder_Start(left_motor.htim_enc, TIM_CHANNEL_ALL);
	left_motor.enc_last_pos = __HAL_TIM_GET_COUNTER(left_motor.htim_enc);
	left_motor.current_speed = 0.0f;
	left_motor.target_speed = 0.0f;
	left_motor.htim_pwm = &htim3;
	left_motor.pwm_channel = TIM_CHANNEL_1;
	__HAL_TIM_SET_COMPARE(left_motor.htim_pwm, left_motor.pwm_channel, 0);
	HAL_TIM_PWM_Start(left_motor.htim_pwm, left_motor.pwm_channel);

	right_motor.htim_enc = &htim5;
	right_motor.dir_port_1 = RMC_RIGHT_MOTOR_IN3_PORT;
	right_motor.dir_pin_1 = RMC_RIGHT_MOTOR_IN3_PIN;
	right_motor.dir_port_2 = RMC_RIGHT_MOTOR_IN4_PORT;
	right_motor.dir_pin_2 = RMC_RIGHT_MOTOR_IN4_PIN;
	HAL_TIM_Encoder_Start(right_motor.htim_enc, TIM_CHANNEL_ALL);
	right_motor.enc_last_pos = __HAL_TIM_GET_COUNTER(right_motor.htim_enc);
	right_motor.current_speed = 0.0f;
	right_motor.target_speed = 0.0f;
	right_motor.htim_pwm = &htim3;
	right_motor.pwm_channel = TIM_CHANNEL_2;
	__HAL_TIM_SET_COMPARE(right_motor.htim_pwm, right_motor.pwm_channel, 0);
	HAL_TIM_PWM_Start(right_motor.htim_pwm, right_motor.pwm_channel);
}

void RMC_Motor_Stop(void) {
	left_motor.target_speed = 0.0f;
	right_motor.target_speed = 0.0f;
	last_command = RDT_CONTROL_STOP;

	__HAL_TIM_SET_COMPARE(left_motor.htim_pwm, left_motor.pwm_channel, 0);
	__HAL_TIM_SET_COMPARE(right_motor.htim_pwm, right_motor.pwm_channel, 0);
}

void RMC_Motor_SpeedUpdate(void) {
	uint32_t enc_current_pos = 0;
	int32_t delta_pos = 0;

	// Left Motor Speed
	enc_current_pos = __HAL_TIM_GET_COUNTER(left_motor.htim_enc);
	delta_pos = (int32_t) (enc_current_pos - left_motor.enc_last_pos);
	left_motor.enc_last_pos = enc_current_pos;
	left_motor.current_speed = (float)delta_pos * RMC_TICKS_TO_MPS;

	// Right Motor Speed
	enc_current_pos = __HAL_TIM_GET_COUNTER(right_motor.htim_enc);
	delta_pos = (int32_t) (enc_current_pos - right_motor.enc_last_pos);
	right_motor.enc_last_pos = enc_current_pos;
	right_motor.current_speed = (float)delta_pos * RMC_TICKS_TO_MPS;
}

static uint8_t last_command = RDT_CONTROL_STOP;

void RMC_Motor_ExecuteCommand_V1(const uint8_t current_command) {
	if (current_command == last_command) {
		return;
	}

	last_command = current_command;

	switch (current_command) {
	   case RDT_CONTROL_FORWARD:
		   RMC_Motor_SetPower(&left_motor, RMC_POWER_FULL);
		   RMC_Motor_SetPower(&right_motor, RMC_POWER_FULL);
		   break;

	   case RDT_CONTROL_FORWARD_LEFT:
		   RMC_Motor_SetPower(&left_motor, RMC_POWER_TURN);
		   RMC_Motor_SetPower(&right_motor, RMC_POWER_FULL);
	   	   break;

	   case RDT_CONTROL_LEFT:
		   RMC_Motor_SetPower(&left_motor, -RMC_POWER_SPIN);
           RMC_Motor_SetPower(&right_motor, RMC_POWER_SPIN);
		   break;


	   case RDT_CONTROL_BACKWARD_LEFT:
		   RMC_Motor_SetPower(&left_motor, -RMC_POWER_TURN);
		   RMC_Motor_SetPower(&right_motor, -RMC_POWER_FULL);
	       break;


	   case RDT_CONTROL_BACKWARD:
		   RMC_Motor_SetPower(&left_motor, -RMC_POWER_FULL);
           RMC_Motor_SetPower(&right_motor, -RMC_POWER_FULL);
	   	   break;


	   case RDT_CONTROL_BACKWARD_RIGHT:
		   RMC_Motor_SetPower(&left_motor, -RMC_POWER_FULL);
           RMC_Motor_SetPower(&right_motor, -RMC_POWER_TURN);
	   	   break;

	   case RDT_CONTROL_RIGHT:
		   RMC_Motor_SetPower(&left_motor, RMC_POWER_SPIN);
		   RMC_Motor_SetPower(&right_motor, -RMC_POWER_SPIN);
	   	   break;


	   case RDT_CONTROL_FORWARD_RIGHT:
		   RMC_Motor_SetPower(&left_motor, RMC_POWER_FULL);
		   RMC_Motor_SetPower(&right_motor, RMC_POWER_TURN);
	   	   break;

	   case RDT_CONTROL_STOP:
		   RMC_Motor_Stop();
	   	   break;

	   default:
		   RMC_Motor_Stop();
		   break;
	}
}

float RMC_Motor_GetSpeed(void) {
	float v_left = left_motor.current_speed;
	float v_right = right_motor.current_speed;

	return (v_left + v_right) / 2.0f;
}

/* ---------------------------------------------------------------------- */

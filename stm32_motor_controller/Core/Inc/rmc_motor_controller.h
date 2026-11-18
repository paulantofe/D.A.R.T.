#ifndef INC_RMC_MOTOR_CONTROLLER_H_
#define INC_RMC_MOTOR_CONTROLLER_H_

#include "stm32f4xx_hal_hardware_config.h"

typedef struct {
	/* Motor Input */
	TIM_HandleTypeDef *htim;      /**< pointer to TIM peripheral in encoder mode */
	uint32_t enc_last_pos;        /**< last position of the encoder */

	/* Motor State */
	float current_speed;          /**< current speed in mps */
	float target_speed;           /**< target speed in mps */

	/* Motor Output */
	TIM_HandleTypeDef *htim_pwm;  /**< pointer to TIM peripheral in PWM mode */
	uint32_t pwm_channel;         /**< PWM Channel of the TIM */
} RMC_Motor_t;


void RMC_Motor_Init(RMC_Motor_t *motor);

void RMC_Motor_SpeedUpdate(RMC_Motor_t *motor);

#endif

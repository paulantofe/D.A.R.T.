/**
 * @file     rmc_motor_controller.h
 * @brief    Rover Motor Controller source file
 *
 * @details  This file provides the APIs needed to control the rover motors.
 */


#ifndef INC_RMC_MOTOR_CONTROLLER_H_
#define INC_RMC_MOTOR_CONTROLLER_H_

/**
 * @brief   Initialize the rover left and right motors.
 */
void RMC_Motor_Init(void);

/**
 * @brief   Update the motors current speed.
 * @details Called every 10ms by the TIM PeriodElapsed callback.
 */
void RMC_Motor_SpeedUpdate(void);

#endif

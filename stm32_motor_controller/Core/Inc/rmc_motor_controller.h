/**
 * @file     rmc_motor_controller.h
 * @brief    Rover Motor Controller header file
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
 * @brief   Stop the motors.
 * @details Sets the PWM output to 0 and resets each motor's target speed.
 */
void RMC_Motor_Stop(void);

/**
 * @brief   Update the motors current speed.
 * @details Called every 10ms by the TIM PeriodElapsed callback.
 */
void RMC_Motor_SpeedUpdate(void);

/**
 * @brief   Execute the motor command received from ESP32.
 * @details Uses the current_command variable from globals.h and outputs it in open loop.
 * @note    This is version 1, intended for tests, and does not implement the PID control loop.
 */
void RMC_Motor_ExecuteCommand_V1(void);

/**
 * @brief   Get the rover current speed in mps.
 * @return  Linear speed of the rover calculated with the formula:
 *               (v_left + v_right) / 2.
 * @note    The 360 on the spot turn is considered with speed 0.
 */
float RMC_Motor_GetSpeed(void);

#endif

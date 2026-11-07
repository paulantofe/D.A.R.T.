/**
 * @file     stm32f4xx_it.h
 * @brief    Interrupts header file.
 *
 * @details  This file declares the Service Routines for different IRQs according to the
 *           Vector Table in startup_stm32f407xx.s.
 */

#ifndef __STM32F4xx_IT_H
#define __STM32F4xx_IT_H

#ifdef __cplusplus
extern "C" {
#endif

void SysTick_Handler(void);

void USART1_IRQHandler(void);

#ifdef __cplusplus
}
#endif

#endif

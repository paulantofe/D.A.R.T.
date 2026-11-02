#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f4xx_hal.h"

/**
 * @brief Handle errors by freezing the board and disabling interrupts
 */
void Error_Handler(void);

/**
 * @brief   Configure system clock to maximum speed.
 *
 * @details Establishes the core and peripheral bus maximum frequencies:
 *            - SYSCLK / HCLK : 168 MHz
 *            - APB1 (PCLK1)  :  42 MHz
 *            - APB2 (PCLK2)  :  84 MHz
 */
void SystemClock_Config(void);

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

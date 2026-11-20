/**
 * @file    ws2812.h
 * @brief   WS2812 Driver header file.
 *
 * @details This file provides all the necessary APIs and macros to control WS2812 LEDs.
 *
 * @note    This driver is developed only for the STM32F407G-DISC1 board and uses a specific set of
 *          peripherals and hardware configurations (TIM4 and DMA1)
 */

#ifndef WS2812_DRIVER_WS2812_H_
#define WS2812_DRIVER_WS2812_H_

#include <stdbool.h>

typedef enum {
	WS_BUSY = 0,
	WS_READY
} WS_State_t;

/* ------------------ WS2812 APIs ------------------ */

/**
 * @brief   Initialize the WS LEDs
 * @details Init the used peripherals in order to control
 *          the WS2812 LEDs (TIM4 in OC PWM and DMA)
 */
void WS_Init(void);

/**
 * @brief   Handle the DMA Stream Finished event.
 * @details Stop PWM and set WS state as ready.
 */
void WS_OnDmaComplete(void);

/**
 * @brief   Close all the LEDs.
 * @return  true if WS was not busy (LEDs are closed).
 *          false if WS was busy (LEDs are not closed).
 */
bool WS_CloseLEDs(void);

/**
 * @brief   Get the State of the WS DMA Stream.
 * @return  WS_BUSY  DMA is busy sending PWM.
 *          WS_READY DMA is idle.
 */
WS_State_t WS_GetState(void);

/* ------------------------------------------------- */

#endif

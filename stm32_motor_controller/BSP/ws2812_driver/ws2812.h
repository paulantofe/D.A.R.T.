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
#include <stdint.h>
#include <rdt_protocol.h>

/* ------------------ General Macros Definitions ------------------ */

#define WS_NUMBER_OF_LEDS        19
#define WS_BITS_PER_LED          24
#define WS_RESET_SLOTS           50   // 50 * 1.25us = 62.5us LOW
#define WS_DATA_LEN              (WS_NUMBER_OF_LEDS * WS_BITS_PER_LED)
#define WS_BUFFER_CAPACITY       (WS_DATA_LEN + WS_RESET_SLOTS)
#define WS_STATUS_LEDS           12
#define WS_HEADLIGHT_LEDS        7
#define WS_HEADLIGHT_BRIGHTNESS  150U
#define WS_HEADLIGHT_ON          1U
#define WS_HEADLIGHT_OFF         0U
#define WS_HEADLIGHT_R           255U
#define WS_HEADLIGHT_G           220U
#define WS_HEADLIGHT_B           180U

#define WS_BIT_1                 59   // ~0.7us HIGH
#define WS_BIT_0                 29   // ~0.35us HIGH

typedef enum {
	WS_BUSY = 0,
	WS_READY
} WS_State_t;

/* ---------------------------------------------------------------- */


/* ------------------ WS2812 APIs ------------------ */

/**
 * @brief   Initialize the WS LEDs
 * @details Init the used peripherals in order to control
 *          the WS2812 LEDs (TIM4 in OC PWM and DMA),
 *          set status LEDs brightness to ~50% and close LEDs.
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
 * @brief   Send a Pixel Packet to WS LEDs.
 */
void WS_SendPacket(void);

/**
 * @brief   Get the State of the WS DMA Stream.
 * @return  WS_BUSY  DMA is busy sending PWM.
 *          WS_READY DMA is idle.
 */
WS_State_t WS_GetState(void);

/**
 * @brief   Set a pixel color.
 * @param[in] pixel_id  number of the pixel [0, 18].
 * @param[in] r Red value [0, 255]
 * @param[in] g Green value [0, 255]
 * @param[in] b Blue value [0, 255]
 * @return   true if WS was not busy (pixel is set).
 *           false if WS was busy (pixel is not set).
 */
bool WS_SetPixel(uint8_t pixel_id, uint8_t r, uint8_t g, uint8_t b);

/**
 * @brief   Set the status LEDs brightness.
 * @details Scale the RGB values according to the brightness
 *          (0 - 0%, 255 - 100%).
 * @param[in] brightness  brightness of the LEDs [0, 255].
 * @note Setting the brightness does not change the current LEDs state.
 *       In order to change it, SetPixel + SendPacket must be called again,
 *       after the brightness change.
 *       Headlight LEDs brightness cannot be changed.
 */
void WS_SetStatusBrightness(uint8_t brightness);

/**
 * @brief   Set the status LEDs color.
 * @param[in] r Red value [0, 255]
 * @param[in] g Green value [0, 255]
 * @param[in] b Blue value [0, 255]
 * @note This is a blocking API as it is waiting for status to be WS_READY.
 */
void WS_SetStatusColor(uint8_t r, uint8_t g, uint8_t b);

/**
 * @brief   Request a status color set.
 * @details Sets a request flag and initializes pending color variables.
 *          Main thread is expected to act on the request flag and set the
 *          status LEDs color.
 * @param[in] r Red value [0, 255]
 * @param[in] g Green value [0, 255]
 * @param[in] b Blue value [0, 255]
 * @note This is API is designed to run in ISRs/ISRs Callbacks.
 */
void WS_RequestStatusColor(uint8_t r, uint8_t g, uint8_t b);

/**
 * @brief   Request headlight control.
 * @details Sets a request flag. Main thread is expected to
 *          act on the request flag and control the headlight LEDs.
 * @param[in] headlight_control  RDT_CONTROL_HEADLIGHT_OFF / RDT_CONTROL_HEADLIGHT_ON macros.
 * @note This is API is designed to run in ISRs/ISRs Callbacks.
 */
void WS_RequestHeadlightControl(uint8_t control_headlight);

/**
 * @brief Handle the status LEDs color update request and
 *        headlight update request.
 * @note This API is designed to be called in main while (1),
 *       not in ISRs.
 */
void WS_ProcessRequest(void);

/**
 * @brief Force a status color.
 * @note This API is designed to be called in Error_Handler and
 *       does not use interrupts (they are disabled in Error_Handler).
 *       If the WS_Init() didn't run correctly, status LEDs may not work.
 */
void WS_ForceStatusColor(uint8_t r, uint8_t g, uint8_t b);

/* ------------------------------------------------- */

#endif

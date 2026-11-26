/**
 * @file    ws2812_driver.h
 * @brief   Public API and data structures for the WS2812 LED driver.
 * @details Provides an STM32 HAL-based interface for configuring and controlling
 *          WS2812 LEDs using a preconfigured timer PWM channel and DMA.
 *
 * @note    The timer, PWM channel, GPIO, and DMA must be configured before
 *          initializing the driver.
 */

#ifndef WS2812_DRIVER_H
#define WS2812_DRIVER_H

#include "stm32f4xx_hal.h"
#include <stdint.h>
#include <stdbool.h>

/* ------------------ General Macros Definitions ------------------ */

/**
 * @defgroup WS_PROTOCOL WS2812 Protocol Definitions
 * @{
 */
#define WS_BITS_PER_LED 24U /**< Number of transmitted bits per WS2812 LED */
/** @} */

#define WS_BYTES_PER_LED       3U
#define WS_BITS_PER_LED        24U

/* ---------------------------------------------------------------- */


/* ------------------ WS2812 Structures Definition ------------------ */

/* Color byte order used by the LED strip */
typedef enum {
	WS_COLOR_ORDER_RGB = 0,    /**< RGB byte order */
	WS_COLOR_ORDER_GRB,        /**< GRB byte order used by most WS2812 LEDs */
	WS_COLOR_ORDER_BGR         /**< BGR byte order */
} WS_ColorOrder_t;

/* User-provided WS2812 hardware and LED configuration */
typedef struct {
    TIM_HandleTypeDef *htim_pwm; /**< pointer to the preconfigured PWM timer */
    uint32_t pwm_channel;        /**< timer channel used for WS2812 output */
    uint16_t bit_0_duty;         /**< compare value for transmitting bit 0 */
    uint16_t bit_1_duty;         /**< compare value for transmitting bit 1 */
    uint16_t reset_slots;        /**< number of zero-duty slots after the data */
    uint16_t led_count;          /**< number of LEDs in the strip */
    uint8_t brightness;          /**< global brightness scale [0, 255] */
    WS_ColorOrder_t color_order; /**< byte order expected by the LEDs */
} WS_Config_t;

/* Main handle structure for the WS2812 driver */
typedef struct {
    WS_Config_t config;            /**< driver configuration */
    uint32_t pixel_data_len;       /**< number of data bytes */
    uint32_t pwm_data_len;         /**< number of entries in the PWM waveform */
    bool is_initialized;           /**< true after successful driver initialization */
    volatile bool transfer_active; /**< true while a PWM/DMA transfer is in progress */
} WS_Handle_t;

/* ------------------------------------------------------------------ */


/* ------------------ WS2812 APIs ------------------ */

/**
 * @brief Initialize the WS2812 driver.
 * @param[in,out] hws  pointer to the WS2812 handle structure with
 *                     desired configuration.
 * @retval HAL_OK      Initialization succeeded.
 * @retval HAL_ERROR   NULL handle or invalid configuration parameter.
 * @note  The PWM timer and DMA must be configured before calling this API.
 */
HAL_StatusTypeDef WS_Init(WS_Handle_t *hws);

/* ------------------------------------------------- */

#endif /* WS2812_DRIVER_H */

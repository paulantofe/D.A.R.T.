/**
 * @file    ws2812_driver.h
 * @brief   Public API and data structures for the WS2812 LED driver.
 * @details Provides an STM32 HAL-based interface for configuring and controlling
 *          WS2812 LEDs using a preconfigured timer PWM channel and DMA.
 *
 * @note    The timer, PWM channel, GPIO, and DMA must be configured before
 *          initializing the driver.
 * @note    The driver support only one instance, hence only one strip of WS2812 LEDs.
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
#define WS_BYTES_PER_LED 3U  /**< Number of color bytes stored per LED */
/** @} */

/* ---------------------------------------------------------------- */


/* ------------------ WS2812 Structures Definition ------------------ */

/* Pixel Color structure */
typedef struct {
	uint8_t red;
	uint8_t green;
	uint8_t blue;
} WS_Color_t;

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
    volatile bool request_pending; /**< true when a transmit request is pending */
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

/**
 * @brief Fill a range of pixels with the specified color.
 * @param[in,out] hws          pointer to an initialized WS2812 handle.
 * @param[in]     start_index  zero-based index of the first pixel.
 * @param[in]     count        number of consecutive pixels to fill.
 * @param[in]     color        color to assign to each selected pixel.
 * @retval HAL_OK      Pixel range filled successfully.
 * @retval HAL_BUSY    A PWM/DMA transfer is currently in progress.
 * @retval HAL_ERROR   NULL pointer, uninitialized handle, empty or invalid range.
 */
HAL_StatusTypeDef WS_FillRange(WS_Handle_t *hws, uint16_t start_index,
		uint16_t count, WS_Color_t color);

/**
 * @brief Clear a range of pixels by setting them to black.
 * @param[in,out] hws          pointer to an initialized WS2812 handle.
 * @param[in]     start_index  zero-based index of the first pixel.
 * @param[in]     count        number of consecutive pixels to clear.
 * @retval HAL_OK      Pixel range cleared successfully.
 * @retval HAL_BUSY    A PWM/DMA transfer is currently in progress.
 * @retval HAL_ERROR   NULL pointer, uninitialized handle, empty or invalid range.
 */
HAL_StatusTypeDef WS_ClearRange(WS_Handle_t *hws, uint16_t start_index, uint16_t count);

/**
 * @brief Transmit the current pixel data and wait for completion.
 * @param[in,out] hws  pointer to an initialized WS2812 handle.
 * @retval HAL_OK      Pixel data transmitted successfully.
 * @retval HAL_BUSY    A PWM/DMA transfer is already in progress.
 * @retval HAL_ERROR   NULL pointer, uninitialized handle, invalid DMA
 *                     configuration, or DMA transfer error.
 * @note  This API polls the DMA hardware and does not require interrupts.
 *        It is suitable for use from an error handler, but must not be
 *        called from an interrupt service routine.
 */
HAL_StatusTypeDef WS_TransmitBlocking(WS_Handle_t *hws);

/**
 * @brief Request a non-blocking pixel-data transmission.
 * @param[in,out] hws  pointer to an initialized WS2812 handle.
 * @retval HAL_OK      Transmission request recorded.
 * @retval HAL_ERROR   NULL pointer or uninitialized handle.
 * @note  This API is safe to call from an ISR. It does not access HAL
 *        peripherals or start a DMA transfer. Requests may be coalesced.
 */
HAL_StatusTypeDef WS_RequestTransmit(WS_Handle_t *hws);

/**
 * @brief Process a pending non-blocking transmission request.
 * @param[in,out] hws  pointer to an initialized WS2812 handle.
 * @retval HAL_OK      No request was pending or DMA started successfully.
 * @retval HAL_BUSY    A PWM/DMA transfer is already in progress.
 * @retval HAL_ERROR   NULL pointer, uninitialized handle, or DMA start failure.
 * @note  Call this API repeatedly from the main thread. It never waits for
 *        DMA completion.
 */
HAL_StatusTypeDef WS_Process(WS_Handle_t *hws);

/**
 * @brief Complete and close a non-blocking DMA transmission.
 * @param[in,out] hws  pointer to the WS2812 handle associated with the DMA transfer.
 * @note  Call this API from the timer PWM pulse-finished callback.
 */
void WS_OnDmaComplete(WS_Handle_t *hws);

/* ------------------------------------------------- */

#endif /* WS2812_DRIVER_H */

/**
 * @file    lighting_service.h
 * @brief   Public API for the rover lighting service.
 * @details Provides a singleton service for controlling the onboard status
 *          LEDs and the WS2812 status/headlight LED strip.
 *
 * @note    The service owns its internal WS2812 driver instance and must be
 *          initialized before any other lighting API is called.
 * @note    WS2812 transmission is non-blocking. Call @ref LS_Process
 *          repeatedly from the application main loop.
 */

#ifndef LIGHTING_LIGHTING_SERVICE_H_
#define LIGHTING_LIGHTING_SERVICE_H_

#include "stm32f4xx_hal.h"
#include "system_state_service.h"
#include <stdint.h>

/* ------------------ General Macros Definitions ------------------ */

/**
 * @defgroup LS_LAYOUT          Lighting Service LED Layout and Commands
 * @{
 */
#define LS_LED_BRIGHTNESS       200U /**< Global WS2812 brightness scale. */
#define LS_STATUS_LED_START     0U   /**< First status LED index. */
#define LS_STATUS_LED_COUNT     12U  /**< Number of status LEDs. */
#define LS_HEADLIGHT_LED_START  (LS_STATUS_LED_START + LS_STATUS_LED_COUNT) /**< First headlight LED index. */
#define LS_HEADLIGHT_LED_COUNT  7U   /**< Number of headlight LEDs. */
#define LS_HEADLIGHT_ON         1U   /**< Command to illuminate headlights. */
#define LS_HEADLIGHT_OFF        0U   /**< Command to clear headlights. */
/** @} */

/* ---------------------------------------------------------------- */


/* ------------------ Public Lighting APIs ------------------ */

/**
 * @brief Initialize the lighting service.
 * @details Configures the onboard status LEDs, WS2812 timer/DMA resources,
 *          and applies the boot indication.
 * @retval HAL_OK      Lighting service initialized successfully.
 * @retval HAL_ERROR   Hardware configuration or WS2812 initialization failed.
 * @note    The service uses one internal WS2812 instance.
 */
HAL_StatusTypeDef LS_Init(void);

/**
 * @brief Set the headlight LED strip state.
 * @param[in] OnorOff @ref LS_HEADLIGHT_ON to illuminate the headlights or
 *                    @ref LS_HEADLIGHT_OFF to clear them.
 * @retval HAL_OK      Headlight state updated and transmission requested.
 * @retval HAL_BUSY    The WS2812 transfer is currently active.
 * @retval HAL_ERROR   Service is uninitialized, command is invalid, or the
 *                     requested LED range is invalid.
 * @note    The transmission is started later by @ref LS_Process.
 */
HAL_StatusTypeDef LS_HeadlightControl(uint8_t OnorOff);

/**
 * @brief Apply the LED indication associated with a system state.
 * @param[in] state System state to indicate.
 * @retval HAL_OK      Onboard and WS2812 indicators updated.
 * @retval HAL_BUSY    The WS2812 transfer is currently active.
 * @retval HAL_ERROR   Service is uninitialized or the state is invalid.
 * @note    System-fault indication is transmitted synchronously; other
 *          states request a non-blocking transmission.
 */
HAL_StatusTypeDef LS_SetStateColor(SS_State_t state);

/**
 * @brief Process a pending WS2812 transmission request.
 * @retval HAL_OK      No request was pending or transmission started.
 * @retval HAL_BUSY    A WS2812 transfer is already active.
 * @retval HAL_ERROR   The service is uninitialized or transmission failed.
 * @note    Call this API repeatedly from the application main loop.
 */
HAL_StatusTypeDef LS_Process(void);

/**
 * @brief Complete an active WS2812 DMA transmission.
 * @note    Call this API from @ref HAL_TIM_PWM_PulseFinishedCallback.
 * @note    This API is intended for interrupt context and performs no
 *          blocking operations.
 */
void LS_OnDmaComplete(void);

/* ----------------------------------------------------------- */


#endif /* LIGHTING_LIGHTING_SERVICE_H_ */

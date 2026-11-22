/**
 * @file     globals.h
 * @brief    Header file for global variables, shared between project source files.
 */

#ifndef GLOBALS_H
#define GLOBALS_H

#include <Arduino.h>
#include "app.h"
#include <stdint.h>

/* ------------------ Shared Application State ------------------ */

/** Current high-level ESP32 application state. */
extern volatile System_State_t system_state;

/** Current ESP32-to-STM32 UART link state. */
extern volatile Link_State_t link_state;

/** Current motor control payload transmitted to the STM32. */
extern volatile uint8_t current_motor_cmd;

/** Current headlight control payload transmitted to the STM32. */
extern volatile uint8_t current_headlight_cmd;

/** Battery charge percentage, or "N/A" when it is unavailable. */
extern String battery_level;

/** Latest decoded rover speed in metres per second. */
extern String speed_mps;

/* -------------------------------------------------------------- */

#endif

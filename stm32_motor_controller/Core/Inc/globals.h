/**
 * @file     globals.h
 * @brief    Header file for global variables, shared between project source files.
 */

#ifndef INC_GLOBALS_H_
#define INC_GLOBALS_H_

#include <stdint.h>
#include <stdbool.h>
#include "app.h"

/* ------------------ Shared Application State ------------------ */

/** Current high-level application state. */
extern volatile System_State_t system_state;

/** Current ESP32-to-STM32 RDT link state. */
extern volatile Link_State_t link_state;

/** Most recently received valid motor-control command. */
extern volatile uint8_t current_motor_command;

/** Most recently received valid headlight-control command. */
extern volatile uint8_t current_headlight_command;

/** Byte storage used by interrupt-driven UART reception. */
extern uint8_t received_byte;

/** True until the first valid ESP32 handshake response is received. */
extern volatile bool boot_handshake;

/* -------------------------------------------------------------- */

#endif

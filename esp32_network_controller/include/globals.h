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

extern uint8_t current_command;

/** Battery charge percentage, or "N/A" when it is unavailable. */
extern String battery_level;

extern String speed_mps;

/** True when a driver currently owns control of the rover. */
extern bool has_active_client;

/** IP address of the current driver. */
extern IPAddress active_client_ip;

/** Timestamp of the current driver's most recent /status request. */
extern volatile uint32_t last_status_request;

/* -------------------------------------------------------------- */

#endif

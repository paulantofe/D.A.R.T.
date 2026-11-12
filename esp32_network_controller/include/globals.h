/**
 * @file     globals.h
 * @brief    Header file for global variables, shared between project source files.
 */

#ifndef GLOBALS_H
#define GLOBALS_H

#include <Arduino.h>
#include "app.h"
#include <stdint.h>

extern volatile System_State_t system_state;

extern volatile Link_State_t link_state;

// Percent value
extern String battery_level;

extern bool has_active_client;
extern IPAddress active_client_ip;

extern volatile uint32_t last_status_request;

#endif
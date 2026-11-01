/**
 * @file     globals.h
 * @brief    Header file for global variables, shared between project source files.
 */

#ifndef GLOBALS_H
#define GLOBALS_H

#include <Arduino.h>

// Battery level info 
extern String rover_battery;

// One-driver-only logic and auto-disconnet variables
extern volatile uint32_t last_command_time;
extern volatile bool has_active_client;

#endif
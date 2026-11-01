#ifndef GLOBALS_H
#define GLOBALS_H

#include <Arduino.h>

String rover_battery;

extern volatile uint32_t last_command_time;
extern volatile bool has_active_client;

#endif
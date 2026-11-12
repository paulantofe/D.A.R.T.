#ifndef SERIAL_PROTOCOL_H
#define SERIAL_PROTOCOL_H

#include <stdint.h>

static constexpr uint8_t SP_STM32_BOOT_READY    = 'R';
static constexpr uint8_t SP_ESP32_BOOT_READY    = 'K';
static constexpr uint8_t SP_STM32_HEARTBEAT     = SP_STM32_BOOT_READY;
static constexpr uint8_t SP_UART_HEARTBEAT      = 0x80;
static constexpr uint8_t SP_SYSTEM_FAULT        = 0x81;
static constexpr uint8_t SP_BATTERY_FRAME       = 'B';
static constexpr uint8_t SP_DRIVER_CONNECTED    = 0x01;
static constexpr uint8_t SP_DRIVER_DISCONNECTED = 0x02;

#endif
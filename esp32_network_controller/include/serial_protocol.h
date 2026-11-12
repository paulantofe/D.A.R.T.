/**
 * @file     serial_protocol.h
 * @brief    ESP32-to-STM32 UART protocol constants.
 *
 * @details  During boot, STM32 sends SP_STM32_BOOT_READY until the ESP32
 *           receives three valid timed bytes, then ESP32 responds with
 *           SP_ESP32_BOOT_READY. During runtime, STM32 sends
 *           SP_STM32_HEARTBEAT and ESP32 responds with one driver-state byte.
 *           System-fault bytes are repeated by the faulted board until reset.
 */

#ifndef SERIAL_PROTOCOL_H
#define SERIAL_PROTOCOL_H

#include <stdint.h>

/** STM32 boot and runtime heartbeat byte. */
static constexpr uint8_t SP_STM32_BOOT_READY    = 'R';
/** ESP32 response that completes the boot handshake. */
static constexpr uint8_t SP_ESP32_BOOT_READY    = 'K';
/** STM32 runtime heartbeat byte. */
static constexpr uint8_t SP_STM32_HEARTBEAT     = SP_STM32_BOOT_READY;
/** Repeated system-fault notification. */
static constexpr uint8_t SP_SYSTEM_FAULT        = 0x81;
/** Start byte for a textual battery-voltage frame. */
static constexpr uint8_t SP_BATTERY_FRAME       = 'B';
/** Runtime response indicating that a driver is active. */
static constexpr uint8_t SP_DRIVER_CONNECTED    = 0x01;
/** Runtime response requesting STM32 failsafe behavior. */
static constexpr uint8_t SP_DRIVER_DISCONNECTED = 0x02;

#endif
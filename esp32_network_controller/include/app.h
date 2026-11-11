#ifndef APP_H
#define APP_H

#include <Arduino.h>
#include <stdint.h>

/**
 * @brief Runtime state of the ESP32 application.
 * @details Describes the current operating mode of the rover application.
 */
typedef enum {
    /** System is idle and ready for normal operation. */
    Idle,
    /** System is actively controlling the rover. */
    Moving,
    /** System is in a movement failsafe state. */
    FailSafe,
    /** A system fault has occurred and requires a reboot. */
    SystemFault,
    /** System is not connected to the STM32 link. */
    Offline
} System_State_t;

typedef enum {
    LINK_OFFLINE,
    LINK_HANDSHAKING,
    LINK_ONLINE
} Link_State_t;

/**
 * @brief Enter the terminal system-fault state.
 * @details Sets the application state to SystemFault and repeatedly transmits
 *          the single-byte 'X' fault indication to the STM32.
 * @note This function does not return and requires the system to be rebooted
 *       to leave the fault state.
 */
void SystemFault_Enter(void);

void LinkHandshake_ProcessByte(const char received, const uint32_t now);
void LinkHandshake_CheckTimeout(const uint32_t now);

void Serial1_Process();

#endif
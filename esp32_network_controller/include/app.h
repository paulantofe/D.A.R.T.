#ifndef APP_H
#define APP_H

#include <Arduino.h>
#include <stdint.h>

/* ------------------ System State Definitions ------------------ */

/**
 * @brief    Runtime state of the ESP32 application.
 * @details  Describes the current operating mode of the rover application.
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

/**
 * @brief    Current state of the STM32 serial link.
 * @details  Describes whether the link is disconnected, negotiating, or
 *           established.
 */
typedef enum {
    /** No valid heartbeat has been received from the STM32. */
    LINK_OFFLINE,
    /** The ESP32 is counting valid timed 'R' messages. */
    LINK_HANDSHAKING,
    /** The handshake completed and the link is active. */
    LINK_ONLINE
} Link_State_t;

/* --------------------------------------------------------------- */


/* ------------------ Application APIs ------------------ */

/**
 * @brief    Initialize the ESP32 application.
 * @details  Initializes the debug serial port, STM32 serial link, camera,
 *           and web server in dependency order.
 * @note     Camera or server initialization failure enters the terminal
 *           SystemFault state.
 */
void ESP_Boot(void);

/**
 * @brief    Enter the terminal system-fault state.
 * @details  Sets the application state to SystemFault and repeatedly
 *           transmits the single-byte 'X' fault indication to the STM32.
 * @note     This function does not return and requires the system to be
 *           rebooted to leave the fault state.
 */
void SystemFault_Enter(void);

/**
 * @brief    Process one received STM32 handshake byte.
 * @details  Counts timed 'R' messages and changes the link state to
 *           LINK_ONLINE after the required sequence. Sends 'H' when the
 *           handshake is established.
 * @param    received  Byte received from the STM32 over Serial1.
 * @param    now       Current millisecond timestamp used for interval
 *                     validation.
 * @note     The caller should invoke this function only for received 'R'
 *           bytes.
 */
void LinkHandshake_ProcessByte(const char received, const uint32_t now);

/**
 * @brief    Detect an expired STM32 heartbeat.
 * @details  Changes the link state to LINK_OFFLINE and resets handshake
 *           progress when no 'R' message has arrived within the timeout.
 * @param    now       Current millisecond timestamp used for timeout
 *                     validation.
 */
void LinkHandshake_CheckTimeout(const uint32_t now);

/**
 * @brief    Update the application state from the link state.
 * @details  Transitions an offline or failed-safe application to Idle when
 *           the STM32 link is online, and forces Idle or Moving into
 *           FailSafe when the link is lost.
 * @note     SystemFault is terminal and is not changed by this function.
 */
void SystemState_Update();

/**
 * @brief    Process bytes received over the STM32 serial link.
 * @details  Dispatches received protocol bytes to the handshake and other
 *           serial handlers, then checks for a link timeout.
 * @note     This function should be called continuously from the main loop.
 */
void Serial1_Process();

String Get_SystemState_String(void);

String Get_LinkState_String(void);

/* ------------------------------------------------------ */

#endif
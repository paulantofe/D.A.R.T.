/**
 * @file     app.h
 * @brief    ESP32 application state and runtime API declarations.
 */

#ifndef APP_H
#define APP_H

#include <Arduino.h>

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
    /** The ESP32 is negotiating the RDT handshake. */
    LINK_HANDSHAKING,
    /** The handshake completed and the link is active. */
    LINK_ONLINE
} Link_State_t;

/* -------------------------------------------------------------- */


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
 * @brief Process pending UART bytes and transmit the current control command.
 *
 * This function is called from the main loop and owns the normal Serial1
 * receive/transmit scheduling.
 */
void Process_Serial1(void);

/**
 * @brief    Enter the terminal system-fault state.
 * @details  Sets the application state to SystemFault and repeatedly
 *           transmits the system-fault indication to the STM32.
 * @note     This function does not return and requires the system to be
 *           rebooted to leave the fault state.
 */
void SystemFault_Enter(void);

/**
 * @brief    Update the application state from the link state.
 * @details  Transitions an offline or failed-safe application to Idle when
 *           the STM32 link is online, and forces Idle or Moving into
 *           FailSafe when the link is lost.
 * @note     SystemFault is terminal and is not changed by this function.
 */
void SystemState_Update(void);

/**
 * @brief    Release the active driver after a status heartbeat timeout.
 * @details  Clears the active driver identity when no status request has
 *           been received within RDT_BROWSER_TIMEOUT_MS.
 */
void DriverConnection_CheckTimeout(void);

/**
 * @brief    Return the current application state as a JSON-friendly string.
 */
String Get_SystemState_String(void);

/**
 * @brief    Return the current STM32 link state as a JSON-friendly string.
 */
String Get_LinkState_String(void);

/* ------------------------------------------------------ */

#endif

#ifndef INC_APP_H_
#define INC_APP_H_

/* ------------------ System State Definitions ------------------ */

/**
 * @brief    Runtime state of the STM32 application.
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
    /** System is not connected to the ESP32 link. */
    Offline
} System_State_t;

/**
 * @brief    Current state of the ESP32 serial link.
 * @details  Describes whether the link is disconnected, negotiating, or
 *           established.
 */
typedef enum {
    /** No valid response has been received from the ESP32. */
    LINK_OFFLINE,
    /** The ESP32 is negotiating the RDT handshake. */
    LINK_HANDSHAKING,
    /** The handshake completed and the link is active. */
    LINK_ONLINE
} Link_State_t;

/* -------------------------------------------------------------- */


/* ------------------ Application APIs ------------------ */

/**
 * @brief Enter the runtime failsafe state.
 * @details Marks the link offline, clears the normal-operation LED, and
 *          enables the failsafe indication. Motor PWM shutdown will be added
 *          when the motor-control API is available.
 */
void Failsafe_Enter(void);

/**
 * @brief Exit the failsafe state after a valid ESP32 handshake.
 * @details Marks the link online and restores the normal-operation LED.
 */
void Failsafe_Exit(void);

/**
 * @brief Periodically transmit STM32 handshake requests.
 * @details Sends RDT_STM_REQ while the ESP32 link is not online.
 */
void Process_UART_Tx(void);

/* ------------------------------------------------------ */


#endif

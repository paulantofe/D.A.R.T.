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

void Failsafe_Enter(void);

void Failsafe_Exit(void);

void Process_UART_Tx(void);

/* ------------------------------------------------------ */


#endif

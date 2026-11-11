#ifndef APP_H
#define APP_H

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

/**
 * @brief Enter the terminal system-fault state.
 * @details Sets the application state to SystemFault and repeatedly transmits
 *          the single-byte 'X' fault indication to the STM32.
 * @note This function does not return and requires the system to be rebooted
 *       to leave the fault state.
 */
void SystemFault_Enter(void);

#endif
/**
 * @file     main.cpp
 * @brief    ESP32 application entry point.
 */

#include <Arduino.h>
#include "app.h"
#include "rdt_link.h"

/* ------------------ Shared Application State ------------------ */

/** Current high-level application state. */
volatile System_State_t system_state = System_State_t::Offline;

/** Current ESP32-to-STM32 RDT link state. */
volatile Link_State_t link_state = Link_State_t::LINK_OFFLINE;

/** Control payload transmitted periodically to the STM32. */
uint8_t current_command = RDT_CONTROL_STOP;

/** Latest decoded battery voltage display value. */
String battery_level = "N/A";

/** Latest decoded rover speed display value. */
String speed_mps = "N/A";

/* -------------------------------------------------------------- */


/* ------------------ ESP32 Runtime ------------------ */

void setup() {
    ESP_Boot();
}

void loop() {
    Process_Serial1();

    RDT_Link_CheckTimeout(millis());

    SystemState_Update();

    DriverConnection_CheckTimeout();

    delay(1);
}

/* ----------------------------------------------------- */
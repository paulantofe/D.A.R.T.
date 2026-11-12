/**
 * @file     main.cpp
 * @brief    ESP32 application entry point.
 */

#include <Arduino.h>
#include "app.h"

/* ------------------ Shared Application State ------------------ */

volatile System_State_t system_state = System_State_t::Offline;

volatile Link_State_t link_state = Link_State_t::LINK_OFFLINE;

String battery_level = "N/A";

/* -------------------------------------------------------------- */


/* ------------------ ESP32 Runtime ------------------ */

void setup() {
    ESP_Boot();
}

void loop() {
    Serial1_Process();

    SystemState_Update();

    LinkHandshake_CheckTimeout(millis());

    DriverConnection_CheckTimeout();

    delay(1);
}

/* ----------------------------------------------------- */
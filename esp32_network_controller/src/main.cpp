#include <Arduino.h>
#include "app.h"

volatile System_State_t system_state = System_State_t::Offline;

volatile Link_State_t link_state = Link_State_t::LINK_OFFLINE;

void setup() {
    ESP_Boot();
}

void loop() {
    Serial1_Process();

    SystemState_Update();

    delay(1);
}
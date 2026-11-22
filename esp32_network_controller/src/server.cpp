/**
 * @file     server.cpp
 * @brief    Source file for server support.
 * 
 * @details  This file provides Server_Init API implementation.
 */

#include "server.h"
#include "stream_server.h"
#include "globals.h"
#include "rdt_link.h"
#include <WiFi.h>
#include <LittleFS.h>
#include <WiFiManager.h>
#include <ESPmDNS.h>
#include <ESPAsyncWebServer.h>

static AsyncWebServer server(80);

/* ------------------ Driver State ------------------ */

// Driver state is accessed by the web-server callbacks and the main loop.
// The project currently assumes these accesses do not overlap.

bool has_active_client = false;
IPAddress active_client_ip = { 0, 0, 0, 0 };

/**
 * @brief Convert a browser command character to an RDT control payload.
 * @param[in] command Browser command character.
 * @return The matching RDT control value, or zero for an unknown command.
 */
static uint8_t Control_FromCommand(char command) {
    switch (command) {
        case 'W': return RDT_CONTROL_FORWARD;
        case 'Q': return RDT_CONTROL_FORWARD_LEFT;
        case 'A': return RDT_CONTROL_LEFT;
        case 'Z': return RDT_CONTROL_BACKWARD_LEFT;
        case 'S': return RDT_CONTROL_BACKWARD;
        case 'C': return RDT_CONTROL_BACKWARD_RIGHT;
        case 'D': return RDT_CONTROL_RIGHT;
        case 'E': return RDT_CONTROL_FORWARD_RIGHT;
        case 'X': return RDT_CONTROL_STOP;
        case 'F': return RDT_CONTROL_HEADLIGHT_ON;
        case 'f': return RDT_CONTROL_HEADLIGHT_OFF;

        default:  return 0U;
    }
}

/* -------------------------------------------------- */


/* ------------------ Server Initialization ------------------ */

esp_err_t Server_Init(void) {
    /* Initialize LittleFS. */
    if (!LittleFS.begin(true)) {
        Serial.println("[SERVER] Critical Error: LittleFS failed.");
        return ESP_FAIL;
    }
    Serial.println("[SERVER] LittleFS initialized successfully.");

    /* Initialize WiFiManager. */
    WiFiManager wm;
    wm.setDebugOutput(false);
    wm.setConfigPortalTimeout(180);

    Serial.println("[SERVER] WiFiManager Active. Waiting connection...");

    if (!wm.autoConnect("D.A.R.T. Rover")) {
        Serial.println("[SERVER] Timeout reached for configuring WiFi. Restarting...");
        delay(3000);
        ESP.restart();
    }

    Serial.println("[SERVER] WiFi connected.");
    WiFi.setSleep(false);
    Serial.println("[SERVER] WiFi sleep disabled for streaming.");
    Serial.print("[SERVER] IP Address: ");
    Serial.println(WiFi.localIP());

    /* Initialize mDNS as dart.local. */
    if (!MDNS.begin("dart")) {
        Serial.println("[SERVER] Error initializing mDNS.");
    } else {
        Serial.println("[SERVER] mDNS active. Access http://dart.local");
    }

    /* Initialize static UI web routes. */
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(LittleFS, "/index.html", "text/html");
    });

    server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(LittleFS, "/style.css", "text/css");
    });

    server.on("/script.js", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(LittleFS, "/script.js", "application/javascript");
    });

    /* Initialize the /control web route. */
    server.on("/control", HTTP_POST, [](AsyncWebServerRequest *request) {
        if (link_state != Link_State_t::LINK_ONLINE) {
            request->send(503, "text/plain", "Service Unavailable");
            return;
        }
        
        IPAddress request_ip = request->client()->remoteIP();

        if (has_active_client && request_ip != active_client_ip) {
            Serial.println("[SECURITY] Forbidden Acces. Rover is controlled by someone else");
            request->send(403, "text/plain", "Forbidden - Rover is busy");
            return;
        }

        if (request->hasParam("val", true)) {
            String command = request->getParam("val", true)->value();

            if (command.length() != RDT_CONTROL_LENGTH) {
                request->send(400, "text/plain", "Invalid Command");
                return;
            }

            const uint8_t motor_cmd = Control_FromCommand(command[0]);
            const uint8_t headlight_cmd = Control_FromCommand(command[1]);

            if (!RDT_Control_Motor_IsValid(motor_cmd) || !RDT_Control_Headlight_IsValid(headlight_cmd)) {
                request->send(400, "text/plain", "Invalid Command");
                return;
            }

            current_motor_cmd = motor_cmd;
            current_headlight_cmd = headlight_cmd;

            if (!has_active_client) {
                has_active_client = true;
                active_client_ip = request_ip;
                last_status_request = millis();
                Serial.println("[SYSTEM] Driver connected. IP: " + request_ip.toString());
            }

            request->send(200, "text/plain", "OK");
        } else {
            request->send(400, "text/plain", "Missing Value");
        }
    });

    /* Initialize the /status web route. */
    server.on("/status", HTTP_GET, [](AsyncWebServerRequest *request) {
        const IPAddress request_ip = request->client()->remoteIP();

        if (has_active_client && request_ip == active_client_ip) {
            last_status_request = millis();
        }
    
        const String client_mode = has_active_client && request_ip == active_client_ip
                                       ? "Driver" : "Spectator";
        const String system_state_string = Get_SystemState_String();
        const String link_state_string = Get_LinkState_String();

        String json = String("{\"RSSI\":") + String(WiFi.RSSI()) + ", ";
        json += String("\"Battery\":\"") + battery_level + "\", ";
        json += String("\"System_State\":\"") + system_state_string + "\", ";
        json += String("\"Link_State\":\"") + link_state_string + "\", ";
        json += String("\"Client_Mode\":\"") + client_mode + "\", ";
        json += String("\"Speed\":\"") + speed_mps + "\", ";
        json += String("\"Active_Clients\":") + String(StreamServer_GetActiveClients());;
        json += "}";

        request->send(200, "application/json", json);
    });

    server.begin();
    Serial.println("[SERVER] Async Web Server Started on port 80.");

    if (StreamServer_Init() != ESP_OK) {
        Serial.println("[SERVER] Warning: Stream server failed to start.");
    } else {
        Serial.println("[SERVER] Stream server started on port 81.");
    }

    return ESP_OK;
}

/* ----------------------------------------------------------- */


/* ------------------ Server Shutdown ------------------ */

void Server_Stop(void) {
    StreamServer_Stop();
    server.end();
    MDNS.end();
    WiFi.disconnect(true, true);
    WiFi.mode(WIFI_OFF);
}

/* ----------------------------------------------------- */
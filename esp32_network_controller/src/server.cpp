/**
 * @file     server.cpp
 * @brief    Single-client WebSocket control/telemetry server (port 80).
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
static AsyncWebSocket ws("/ws");

static constexpr uint32_t STATUS_INTERVAL_MS = 300;
static constexpr uint32_t WS_CLEANUP_MS      = 500;
static constexpr uint32_t CLIENT_TIMEOUT_MS  = 1500;

/* ------------------ Static Helper Functions ------------------ */

static inline const char *ToCStr(const char *s) { return s ? s : ""; }
static inline const char *ToCStr(const String &s) { return s.c_str(); }

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

/* ------------------------------------------------------------- */


/* ------------------ Single Client State ------------------ */

static bool has_active_client = false;
IPAddress active_client_ip = { 0, 0, 0, 0 };

static uint32_t active_client_id = 0;
static uint32_t last_client_activity_ms = 0;
static uint32_t active_client_ip_raw = 0;
static portMUX_TYPE client_mux = portMUX_INITIALIZER_UNLOCKED;

bool Server_IsActiveClientIP(uint32_t ipv4_addr) {
    portENTER_CRITICAL(&client_mux);
    const bool match = (active_client_id != 0) && (active_client_ip_raw == ipv4_addr);
    portEXIT_CRITICAL(&client_mux);
    return match;
}

static uint32_t ClearActiveClient(void) {
    portENTER_CRITICAL(&client_mux);
    const uint32_t old_id = active_client_id;
    active_client_id = 0;
    active_client_ip_raw = 0;
    has_active_client = false;
    active_client_ip = IPAddress(0, 0, 0, 0);
    current_motor_cmd = RDT_CONTROL_FAILSAFE;
    portEXIT_CRITICAL(&client_mux);
    return old_id;
}

static void SendWsError(AsyncWebSocketClient *client, uint16_t code, const char *msg) {
    if (!client || !client->canSend()) {
        return;
    }
    char buf[96];
    const int len = snprintf(
        buf, sizeof(buf),
        "{\"type\":\"error\",\"code\":%u,\"msg\":\"%s\"}",
        code, msg
    );
    if (len > 0 && static_cast<size_t>(len) < sizeof(buf)) {
        client->text(buf, static_cast<size_t>(len));
    }
}

/* --------------------------------------------------------- */


/* ------------------ WebSocket Handler ------------------ */

static void HandleWsMessage(AsyncWebSocketClient *client, const uint8_t *data, size_t len) {
    const uint32_t client_id = client->id();

    if (len == 1 && data[0] == 'P') {
        portENTER_CRITICAL(&client_mux);
        if (active_client_id == client_id) {
            last_client_activity_ms = millis();
        }
        portEXIT_CRITICAL(&client_mux);
        return;
    }

    if (len != RDT_CONTROL_LENGTH) {
        SendWsError(client, 400, "Invalid Command");
        return;
    }

    if (link_state != Link_State_t::LINK_ONLINE) {
        SendWsError(client, 503, "Service Unavailable");
        return;
    }

    const uint8_t motor_cmd = Control_FromCommand(static_cast<char>(data[0]));
    const uint8_t headlight_cmd = Control_FromCommand(static_cast<char>(data[1]));

    if (!RDT_Control_Motor_IsValid(motor_cmd) || !RDT_Control_Headlight_IsValid(headlight_cmd)) {
        SendWsError(client, 400, "Invalid Command");
        return;
    }

    bool authorized = false;
    portENTER_CRITICAL(&client_mux);
    if (active_client_id == client_id) {
        current_motor_cmd = motor_cmd;
        current_headlight_cmd = headlight_cmd;
        last_client_activity_ms = millis();
        authorized = true;
    }
    portEXIT_CRITICAL(&client_mux);

    if (!authorized) {
        client->close(4003, "Rover is busy");
    }
}

static void OnWsEvent(
    AsyncWebSocket *server_ws,
    AsyncWebSocketClient *client,
    AwsEventType type,
    void *arg,
    uint8_t *data,
    size_t len
) {
    (void)server_ws;

    switch (type) {
        case WS_EVT_CONNECT: {
            bool accepted = false;
            const IPAddress ip = client->remoteIP();

            portENTER_CRITICAL(&client_mux);
            if (active_client_id == 0) {
                active_client_id = client->id();
                has_active_client = true;
                active_client_ip = ip;
                active_client_ip_raw = static_cast<uint32_t>(ip);
                last_client_activity_ms = millis();
                accepted = true;
            }
            portEXIT_CRITICAL(&client_mux);

            if (!accepted) {
                client->close(4003, "Rover is busy");
                return;
            }

            if (client->client()) {
                client->client()->setNoDelay(true);
            }

            client->text("{\"type\":\"ready\"}");
            Serial.printf("[SYSTEM] Client connected (ID: %lu, IP: %s).\n",
                          static_cast<unsigned long>(client->id()),
                          ip.toString().c_str());
            break;
        }

        case WS_EVT_DISCONNECT:
        case WS_EVT_ERROR: {
            bool was_active = false;
            portENTER_CRITICAL(&client_mux);
            if (active_client_id == client->id()) {
                was_active = true;
            }
            portEXIT_CRITICAL(&client_mux);

            if (was_active) {
                ClearActiveClient();
                Serial.printf("[SYSTEM] Client disconnected (ID: %lu). Rover stopped.\n",
                              static_cast<unsigned long>(client->id()));
            }
            break;
        }

        case WS_EVT_DATA: {
            const AwsFrameInfo *info = reinterpret_cast<const AwsFrameInfo *>(arg);
            if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
                HandleWsMessage(client, data, len);
            }
            break;
        }

        case WS_EVT_PONG:
        default:
            break;
    }
}

/* ------------------------------------------------------- */


/* ------------------ Status Push ------------------ */

static void SendStatusToActiveClient(void) {
    portENTER_CRITICAL(&client_mux);
    const uint32_t client_id = active_client_id;
    portEXIT_CRITICAL(&client_mux);

    if (client_id == 0 || !ws.availableForWrite(client_id)) {
        return;
    }

    const auto sys_state = Get_SystemState_String();
    const auto lnk_state = Get_LinkState_String();

    char json[200];
    const int len = snprintf(
        json, sizeof(json),
        "{\"type\":\"status\","
        "\"RSSI\":%d,"
        "\"Battery\":\"%s\","
        "\"System_State\":\"%s\","
        "\"Link_State\":\"%s\","
        "\"Speed\":\"%s\"}",
        WiFi.RSSI(),
        ToCStr(battery_level),
        ToCStr(sys_state),
        ToCStr(lnk_state),
        ToCStr(speed_mps)
    );

    if (len > 0 && static_cast<size_t>(len) < sizeof(json)) {
        ws.text(client_id, json, static_cast<size_t>(len));
    }
}

/* ------------------------------------------------- */


/* ------------------ Public API ------------------ */

esp_err_t Server_Init(void) {
    if (!LittleFS.begin(true)) {
        Serial.println("[SERVER] Critical Error: LittleFS failed.");
        return ESP_FAIL;
    } else {
        Serial.println("[SERVER] LittleFS mounted successfully.");
    }

    WiFiManager wm;
    wm.setDebugOutput(false);
    wm.setConfigPortalTimeout(180);

    if (!wm.autoConnect("D.A.R.T. Rover")) {
        delay(3000);
        ESP.restart();
    }

    WiFi.setSleep(false);
    Serial.print("[SERVER] IP Address: ");
    Serial.println(WiFi.localIP());

    if (!MDNS.begin("dart")) {
        Serial.println("[SERVER] Error initializing mDNS.");
    } else {
        Serial.println("[SERVER] mDNS started. Access the rover at http://dart.local");
    }

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(LittleFS, "/index.html", "text/html");
    });

    server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(LittleFS, "/style.css", "text/css");
    });

    server.on("/script.js", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(LittleFS, "/script.js", "application/javascript");
    });

    ws.onEvent(OnWsEvent);
    server.addHandler(&ws);

    server.begin();
    Serial.println("[SERVER] Single-client WebSocket server started on port 80.");

    if (StreamServer_Init() != ESP_OK) {
        Serial.println("[SERVER] Warning: Stream server failed to start.");
    }

    return ESP_OK;
}

void Server_Tick(void) {
    static uint32_t last_cleanup_ms = 0;
    static uint32_t last_status_ms = 0;
    const uint32_t now = millis();

    if (now - last_cleanup_ms >= WS_CLEANUP_MS) {
        last_cleanup_ms = now;
        ws.cleanupClients();
    }

    uint32_t timed_out_id = 0;
    portENTER_CRITICAL(&client_mux);
    if (active_client_id != 0 && (now - last_client_activity_ms >= CLIENT_TIMEOUT_MS)) {
        timed_out_id = active_client_id;
    }
    portEXIT_CRITICAL(&client_mux);

    if (timed_out_id != 0) {
        ClearActiveClient();
        ws.close(timed_out_id, 4008, "Heartbeat timeout");
        Serial.println("[SYSTEM] Client heartbeat timeout. Rover stopped.");
    }

    if (now - last_status_ms >= STATUS_INTERVAL_MS) {
        last_status_ms = now;
        SendStatusToActiveClient();
    }
}

void Server_Stop(void) {
    ClearActiveClient();
    ws.closeAll();
    StreamServer_Stop();
    server.end();
    MDNS.end();
    WiFi.disconnect(true, true);
    WiFi.mode(WIFI_OFF);
}

/* ------------------------------------------------ */
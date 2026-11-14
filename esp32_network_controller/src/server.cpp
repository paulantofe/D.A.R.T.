/**
 * @file     server.cpp
 * @brief    Source file for server support.
 * 
 * @details  This file provides Server_Init API implementation.
 */

#include "server.h"
#include "esp_camera.h"
#include "globals.h"
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

/* -------------------------------------------------- */


/* ------------------ Stream Client Tracking ------------------ */

static uint32_t active_stream_clients = 0;
static portMUX_TYPE active_stream_clients_mux =
    portMUX_INITIALIZER_UNLOCKED;

/**
 * @brief    Return the number of active MJPEG stream connections.
 */
static uint32_t GetActiveStreamClients(void) {
    portENTER_CRITICAL(&active_stream_clients_mux);
    const uint32_t count = active_stream_clients;
    portEXIT_CRITICAL(&active_stream_clients_mux);
    return count;
}

/**
 * @brief    Increment the active MJPEG stream connection count.
 */
static void IncrementActiveStreamClients(void) {
    portENTER_CRITICAL(&active_stream_clients_mux);
    ++active_stream_clients;
    portEXIT_CRITICAL(&active_stream_clients_mux);
}

/**
 * @brief    Decrement the active MJPEG stream connection count.
 */
static void DecrementActiveStreamClients(void) {
    portENTER_CRITICAL(&active_stream_clients_mux);
    if (active_stream_clients > 0) {
        --active_stream_clients;
    }
    portEXIT_CRITICAL(&active_stream_clients_mux);
}

/* ------------------------------------------------------------ */


/* ------------------ Camera Stream Response ------------------ */

/**
 * @brief    Asynchronous multipart response for the camera MJPEG stream.
 * @details  Captures camera frames on demand and releases each frame after
 *           its contents have been sent.
 */
class AsyncJpegStreamResponse : public AsyncAbstractResponse {
private:
    camera_fb_t *fb;
    size_t index;
    char header[128];
    size_t header_len;
    size_t header_index;

public:
    AsyncJpegStreamResponse() {
        _code = 200;
        _contentType = "multipart/x-mixed-replace; boundary=frame";
        _sendContentLength = false;
        fb = nullptr;
        index = 0;
        header_len = 0;
        header_index = 0;
        IncrementActiveStreamClients();
    }

    ~AsyncJpegStreamResponse() {
        if (fb) esp_camera_fb_return(fb);
        DecrementActiveStreamClients();
    }

    bool _sourceValid() const { return true; }

    size_t _fillBuffer(uint8_t *buf, size_t maxLen) override {
        if (!fb) {
            fb = esp_camera_fb_get();
            if (!fb) {
                Serial.println("[CAMERA] Frame capture failed.");
                return RESPONSE_TRY_AGAIN;
            }

            index = 0;
            header_index = 0;
            header_len = snprintf(
                header,
                sizeof(header),
                "\r\n--frame\r\n"
                "Content-Type: image/jpeg\r\n"
                "Content-Length: %zu\r\n\r\n",
                fb->len
            );

            if (header_len >= sizeof(header)) {
                esp_camera_fb_return(fb);
                fb = nullptr;
                return RESPONSE_TRY_AGAIN;
            }
        }

        size_t bytes_sent = 0;

        if (header_index < header_len && maxLen > 0) {
            size_t header_bytes = header_len - header_index;
            if (header_bytes > maxLen) {
                header_bytes = maxLen;
            }

            memcpy(buf, header + header_index, header_bytes);
            header_index += header_bytes;
            buf += header_bytes;
            maxLen -= header_bytes;
            bytes_sent += header_bytes;
        }

        if (maxLen > 0 && index < fb->len) {
            size_t frame_bytes = fb->len - index;
            if (frame_bytes > maxLen) {
                frame_bytes = maxLen;
            }

            memcpy(buf, fb->buf + index, frame_bytes);
            index += frame_bytes;
            bytes_sent += frame_bytes;
        }

        if (header_index == header_len && index == fb->len) {
            esp_camera_fb_return(fb);
            fb = nullptr;
        }

        return bytes_sent;
    }
};

/* ------------------------------------------------------------ */


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

            if (command.length() == 0) {
                request->send(400, "text/plain", "Empty Command Value");
                return;
            }

            request->send(
                501,
                "text/plain",
                "RDT control transmission is not implemented"
            );
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
        json += String("\"Active_Clients\":") + String(GetActiveStreamClients());
        json += "}";

        request->send(200, "application/json", json);
    });

    /* Initialize the /stream web route. */
    server.on("/stream", HTTP_GET, [](AsyncWebServerRequest *request) {
        AsyncJpegStreamResponse *response = new AsyncJpegStreamResponse();
        request->send(response);
    });

    server.begin();
    Serial.println("[SERVER] Async Web Server Started on port 80.");

    return ESP_OK;
}

/* ----------------------------------------------------------- */


/* ------------------ Server Shutdown ------------------ */

void Server_Stop(void) {
    server.end();
    MDNS.end();
    WiFi.disconnect(true, true);
    WiFi.mode(WIFI_OFF);
}

/* ----------------------------------------------------- */
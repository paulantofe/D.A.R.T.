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

AsyncWebServer server(80);

IPAddress active_client_ip;
uint32_t active_stream_clients = 0;
portMUX_TYPE active_stream_clients_mux = portMUX_INITIALIZER_UNLOCKED;

uint32_t GetActiveStreamClients(void) {
    portENTER_CRITICAL(&active_stream_clients_mux);
    const uint32_t count = active_stream_clients;
    portEXIT_CRITICAL(&active_stream_clients_mux);
    return count;
}

void IncrementActiveStreamClients(void) {
    portENTER_CRITICAL(&active_stream_clients_mux);
    ++active_stream_clients;
    portEXIT_CRITICAL(&active_stream_clients_mux);
}

void DecrementActiveStreamClients(void) {
    portENTER_CRITICAL(&active_stream_clients_mux);
    if (active_stream_clients > 0) {
        --active_stream_clients;
    }
    portEXIT_CRITICAL(&active_stream_clients_mux);
}

// Response class for camera live stream GET request
class AsyncJpegStreamResponse : public AsyncAbstractResponse {
private:
    camera_fb_t *fb;
    size_t index;

public:
    AsyncJpegStreamResponse() {
        _code = 200;
        _contentType = "multipart/x-mixed-replace; boundary=frame";
        _sendContentLength = false;
        fb = NULL;
        index = 0;
        IncrementActiveStreamClients();
    }

    ~AsyncJpegStreamResponse() {
        if (fb) esp_camera_fb_return(fb);
        DecrementActiveStreamClients();
    }

    bool _sourceValid() const { return true; }

    size_t _fillBuffer(uint8_t *buf, size_t maxLen) override {
        size_t bytes_sent = 0;
        
        if (!fb) {
            fb = esp_camera_fb_get();
            if (!fb) {
                Serial.println("[CAMERA] Frame capture failed.");
                return RESPONSE_TRY_AGAIN;
            }
            index = 0;
            
            char header[128];
            size_t hlen = snprintf(header, sizeof(header), 
                "\r\n--frame\r\nContent-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n", 
                fb->len);
                
            if (maxLen > hlen) {
                memcpy(buf, header, hlen);
                buf += hlen;
                maxLen -= hlen;
                bytes_sent += hlen;
            } else {
                esp_camera_fb_return(fb);
                fb = NULL;
                return 0; 
            }
        }

        size_t to_copy = fb->len - index;
        if (to_copy > maxLen) {
            to_copy = maxLen; 
        }

        memcpy(buf, fb->buf + index, to_copy);
        index += to_copy;
        bytes_sent += to_copy;

        if (index == fb->len) {
            esp_camera_fb_return(fb);
            fb = NULL;
        }
        
        return bytes_sent;
    }
       
};

// Initialization API implementation
esp_err_t Server_Init(void) {
    // Initialize LittleFS
    if (!LittleFS.begin(true)) {
        Serial.println("[SERVER] Critical Error: LittleFS failed.");
        return ESP_FAIL;
    }
    Serial.println("[SERVER] LittleFS initialized successfully.");

    // Initialize WiFiManager
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

    // Initialize mDNS
    // Domain name: "dart.local"
    if (!MDNS.begin("dart")) {
        Serial.println("[SERVER] Error initializing mDNS.");
    } else {
        Serial.println("[SERVER] mDNS active. Access http://dart.local");
    }

    // Initialize Static UI Web Routes
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(LittleFS, "/index.html", "text/html");
    });

    server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(LittleFS, "/style.css", "text/css");
    });

    server.on("/script.js", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(LittleFS, "/script.js", "application/javascript");
    });

    // Initialize /control Web Route
    server.on("/control", HTTP_POST, [](AsyncWebServerRequest *request) {
        IPAddress request_ip = request->client()->remoteIP();

        if (has_active_client && request_ip != active_client_ip) {
            Serial.println("[SECURITY] Forbidden Acces. Rover is controlled by someone else");
            request->send(403, "text/plain", "Forbidden - Rover is busy");
            return;
        }

        last_command_time = millis();

        if (!has_active_client) {
            has_active_client = true;
            active_client_ip = request_ip;
            Serial.print("[SERVER] New driver Connected: ");
            Serial.println(active_client_ip);
        }
        
        if (request->hasParam("val", true)) {
            String command = request->getParam("val", true)->value();

            if (command.length() > 0) {
                Serial1.print(command[0]);
            }

            request->send(200, "text/plain", "OK");
        } else {
            request->send(400, "text/plain", "Missing Value");
        }
    });

    // Initialize /status Web Route
    server.on("/status", HTTP_GET, [](AsyncWebServerRequest *request) {
        IPAddress request_ip = request->client()->remoteIP();
        String mode;

        if (has_active_client && active_client_ip == request_ip) {
            last_command_time = millis();
            mode = "Driver";
        }
        else {
            mode = "Spectator";
        }
        
        int8_t rssi = WiFi.RSSI();
        String json = "{";
        json += "\"battery\":\"" + rover_battery + "\",";
        json += "\"rssi\":" + String(rssi) + ",";
        json += "\"mode\":\"" + mode + "\",";
        json += "\"stm32_link_known\":";
        json += stm32_link_status_known ? "true" : "false";
        json += ",";
        json += "\"stm32_link_ok\":";
        json += stm32_link_ok ? "true" : "false";
        json += ",";
        json += "\"stream_clients\":" + String(GetActiveStreamClients());
        json += "}";

        request->send(200, "application/json", json);
    });

    // Initialize /stream Web Route
    server.on("/stream", HTTP_GET, [](AsyncWebServerRequest *request) {
        AsyncJpegStreamResponse *response = new AsyncJpegStreamResponse();
        request->send(response);
    });

    server.begin();
    Serial.println("[SERVER] Async Web Server Started on port 80.");

    return ESP_OK;
}

void Server_Stop(void) {
    server.end();
    MDNS.end();
    WiFi.disconnect(true, true);
    WiFi.mode(WIFI_OFF);
}
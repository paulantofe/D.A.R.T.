#include "server.h"
#include <LittleFS.h>
#include <WiFiManager.h>
#include <ESPmDNS.h>
#include <ESPAsyncWebServer.h>

AsyncWebServer server(80);

extern String rover_battery;

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
    }

    ~AsyncJpegStreamResponse() {
        if (fb) esp_camera_fb_return(fb);
    }

    bool _sourceValid() const { return true; }

    size_t _fillBuffer(uint8_t *buf, size_t maxLen) override {
        size_t bytes_sent = 0;
        
        if (!fb) {
            fb = esp_camera_fb_get();
            if (!fb) {
                Serial.println("[CAMERA] Frame capture failed.");
                return 0; 
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
        int rssi = WiFi.RSSI();
        String json = "{";
        json += "\"battery\":\"" + rover_battery + "\",";
        json += "\"rssi\":" + String(rssi);
        json += "}";

        request->send(200, "application/json", json);
    });

    // Initialize /stream Web Route
    server.on("/stream", HTTP_GET, [](AsyncWebServerRequest *request) {
        AsyncJpegStreamResponse *response = new AsyncJpegStreamResponse;

        request->send(response);
    });

    return ESP_OK;
}
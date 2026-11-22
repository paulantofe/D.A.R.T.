/**
 * @file    stream_server.cpp
 * @brief   Dedicated native esp_http_server for MJPEG streaming on port 81.
 */

 #include "stream_server.h"
 #include "esp_camera.h"
 #include "esp_http_server.h"
 #include "Arduino.h"

static constexpr uint16_t STREAM_SERVER_PORT = 81;
static constexpr uint16_t STREAM_CTRL_PORT   = 32769;
static constexpr uint8_t  MAX_CAPTURE_FAILS  = 10;

static const char *STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=frame";
static const char *STREAM_PART_FMT =
    "\r\n--frame\r\n"
    "Content-Type: image/jpeg\r\n"
    "Content-Length: %zu\r\n\r\n";

static httpd_handle_t stream_httpd = nullptr;
static volatile bool stream_running = false;


/* ------------------ Stream Client Tracking ------------------ */

static uint32_t active_stream_clients = 0;
static portMUX_TYPE active_stream_clients_mux = portMUX_INITIALIZER_UNLOCKED;

static void IncrementActiveStreamClients(void) {
    portENTER_CRITICAL(&active_stream_clients_mux);
    ++active_stream_clients;
    portEXIT_CRITICAL(&active_stream_clients_mux);
}

static void DecrementActiveStreamClients(void) {
    portENTER_CRITICAL(&active_stream_clients_mux);
    if (active_stream_clients > 0) {
        --active_stream_clients;
    }
    portEXIT_CRITICAL(&active_stream_clients_mux);
}

/* ------------------------------------------------------------- */


/* ------------------ MJPEG Stream Handler ------------------ */

static esp_err_t Stream_Handler(httpd_req_t *req) {
    esp_err_t res = httpd_resp_set_type(req, STREAM_CONTENT_TYPE);
    if (res != ESP_OK) {
        return res;
    }

    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store, no-cache, must-revalidate");

    IncrementActiveStreamClients();

    char part_buf[128];
    uint8_t consecutive_fails = 0;

    while (stream_running) {
        camera_fb_t *fb = esp_camera_fb_get();
        if (!fb) {
            if (++consecutive_fails >= MAX_CAPTURE_FAILS) {
                Serial.println("[STREAM] Camera capture failed repeatedly. Closing stream.");
                res = ESP_FAIL;
                break;
            }
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }
        consecutive_fails = 0;

        const int hlen = snprintf(part_buf, sizeof(part_buf), STREAM_PART_FMT, fb->len);
        if (hlen < 0 || static_cast<size_t>(hlen) >= sizeof(part_buf)) {
            esp_camera_fb_return(fb);
            res = ESP_FAIL;
            break;
        }

        res = httpd_resp_send_chunk(req, part_buf, static_cast<ssize_t>(hlen));
        if (res == ESP_OK) {
            res = httpd_resp_send_chunk(
                req,
                reinterpret_cast<const char *>(fb->buf),
                static_cast<ssize_t>(fb->len)
            );
        }

        esp_camera_fb_return(fb);

        if (res != ESP_OK) {
            break;
        }
    }

    DecrementActiveStreamClients();
    return res;
}

/* ---------------------------------------------------------- */


/* ------------------ Public API ------------------ */

esp_err_t StreamServer_Init(void) {
    if (stream_httpd != nullptr) {
        return ESP_OK;
    }

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = STREAM_SERVER_PORT;
    config.ctrl_port = STREAM_CTRL_PORT;
    config.lru_purge_enable = true;
    config.stack_size = 4096;
    config.task_priority = 4; 

#if !CONFIG_FREERTOS_UNICORE
    config.core_id = 1; 
#endif

    const httpd_uri_t stream_uri = {
        .uri      = "/stream",
        .method   = HTTP_GET,
        .handler  = Stream_Handler,
        .user_ctx = nullptr
    };

    stream_running = true;
    const esp_err_t err = httpd_start(&stream_httpd, &config);
    if (err != ESP_OK) {
        stream_running = false;
        stream_httpd = nullptr;
        Serial.printf("[STREAM] Failed to start server on port %u (err=0x%x)\n",
                      STREAM_SERVER_PORT, err);
        return err;
    }

    httpd_register_uri_handler(stream_httpd, &stream_uri);
    Serial.printf("[STREAM] MJPEG server started on port %u.\n", STREAM_SERVER_PORT);
    return ESP_OK;
}

void StreamServer_Stop(void) {
    stream_running = false;
    if (stream_httpd != nullptr) {
        httpd_stop(stream_httpd);
        stream_httpd = nullptr;
        Serial.println("[STREAM] MJPEG server stopped.");
    }
}

uint32_t StreamServer_GetActiveClients(void) {
    portENTER_CRITICAL(&active_stream_clients_mux);
    const uint32_t count = active_stream_clients;
    portEXIT_CRITICAL(&active_stream_clients_mux);
    return count;
}

/* ------------------------------------------------ */
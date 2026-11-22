/**
 * @file    stream_server.cpp
 * @brief   Dedicated native esp_http_server for MJPEG streaming on port 81.
 */

 #include <Arduino.h>
 #include "stream_server.h"
 #include "server.h"
 #include "esp_camera.h"
 #include "esp_http_server.h"
 #include <lwip/sockets.h>

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

static bool GetClientIPv4(httpd_req_t *req, uint32_t *out_ipv4) {
    const int sockfd = httpd_req_to_sockfd(req);
    if (sockfd < 0) {
        return false;
    }

    struct sockaddr_storage addr;
    socklen_t addr_len = sizeof(addr);
    if (lwip_getpeername(sockfd, reinterpret_cast<struct sockaddr *>(&addr), &addr_len) != 0) {
        return false;
    }

    if (addr.ss_family == AF_INET) {
        const auto *s = reinterpret_cast<const struct sockaddr_in *>(&addr);
        *out_ipv4 = s->sin_addr.s_addr;
        return true;
    }

    if (addr.ss_family == AF_INET6) {
        const auto *s6 = reinterpret_cast<const struct sockaddr_in6 *>(&addr);
        if (s6->sin6_addr.un.u32_addr[0] == 0 &&
            s6->sin6_addr.un.u32_addr[1] == 0 &&
            s6->sin6_addr.un.u32_addr[2] == htonl(0x0000FFFF)) {
            *out_ipv4 = s6->sin6_addr.un.u32_addr[3];
            return true;
        }
    }

    return false;
}

/* ------------------------------------------------------------- */


/* ------------------ MJPEG Stream Handler ------------------ */

static esp_err_t Stream_Handler(httpd_req_t *req) {
    uint32_t client_ipv4 = 0;
    if (!GetClientIPv4(req, &client_ipv4) || !Server_IsActiveClientIP(client_ipv4)) {
        httpd_resp_send_err(req, HTTPD_403_FORBIDDEN, "Rover is busy");
        return ESP_FAIL;
    }

    esp_err_t res = httpd_resp_set_type(req, STREAM_CONTENT_TYPE);
    if (res != ESP_OK) {
        return res;
    }

    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store, no-cache, must-revalidate");

    char part_buf[128];
    uint8_t consecutive_fails = 0;

    while (stream_running && Server_IsActiveClientIP(client_ipv4)) {
        camera_fb_t *fb = esp_camera_fb_get();
        if (!fb) {
            if (++consecutive_fails >= MAX_CAPTURE_FAILS) {
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
        return err;
    }

    httpd_register_uri_handler(stream_httpd, &stream_uri);
    Serial.printf("[STREAM] Single-client MJPEG server started on port %u.\n", STREAM_SERVER_PORT);
    return ESP_OK;
}

void StreamServer_Stop(void) {
    stream_running = false;
    if (stream_httpd != nullptr) {
        httpd_stop(stream_httpd);
        stream_httpd = nullptr;
    }
}

/* ------------------------------------------------ */
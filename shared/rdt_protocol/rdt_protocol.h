/**
 * @file     rdt_protocol.h
 * @brief    Shared Rover Data Transport protocol definitions.
 */

#ifndef RDT_PROTOCOL_H
#define RDT_PROTOCOL_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------ RDT Protocol Macro Definitions ------------------ */

/**
 * @defgroup RDT_Frame RDT frame constants
 * @{
 */

#define RDT_START_BYTE       0xAAU
#define RDT_ESCAPE_BYTE      0x7DU
#define RDT_CRC8_POLYNOMIAL  0x07U

/** @} */

/**
 * @defgroup RDT_MessageTypes RDT message types and lengths
 * @{
 */

#define RDT_CONTROL_TYPE     0x01U
#define RDT_STATUS_TYPE      0x02U
#define RDT_HANDSHAKE_TYPE   0x03U

#define RDT_CONTROL_LENGTH   0x01U
#define RDT_STATUS_LENGTH    0x08U
#define RDT_HANDSHAKE_LENGTH 0x01U

/** @} */

/**
 * @defgroup RDT_Handshake RDT handshake constants
 * @{
 */

#define RDT_STM_REQ                     0x60U
#define RDT_ESP_OK                      0x61U
#define RDT_REQUIRED_VALID_REQUESTS     3U
#define RDT_STM_REQUEST_PERIOD_MS       50U
#define RDT_REQUEST_INTERVAL_TIMEOUT_MS 100U
#define RDT_HANDSHAKE_TIMEOUT_MS        1000U

/** @} */

/**
 * @defgroup RDT_Watchdogs RDT watchdog timing constants
 * @{
 */

#define RDT_ESP_CONTROL_PERIOD_MS       50U
#define RDT_STM_CONTROL_TIMEOUT_MS      200U
#define RDT_STM_STATUS_PERIOD_MS        500U
#define RDT_ESP_STATUS_TIMEOUT_MS       1500U
#define RDT_BROWSER_REQUEST_PERIOD_MS   500U
#define RDT_BROWSER_TIMEOUT_MS          1500U

/** @} */

/**
 * @defgroup RDT_ControlPayloads RDT control payload values
 * @{
 */

#define RDT_CONTROL_FORWARD             0x80U
#define RDT_CONTROL_FORWARD_LEFT        0x81U
#define RDT_CONTROL_LEFT                0x82U
#define RDT_CONTROL_BACKWARD_LEFT       0x83U
#define RDT_CONTROL_BACKWARD            0x84U
#define RDT_CONTROL_BACKWARD_RIGHT      0x85U
#define RDT_CONTROL_RIGHT               0x86U
#define RDT_CONTROL_FORWARD_RIGHT       0x87U
#define RDT_CONTROL_STOP                0x88U
#define RDT_CONTROL_SYSTEM_FAULT        0x91U
#define RDT_CONTROL_FAILSAFE            0x92U

/** @} */

/* -------------------------------------------------------------------- */

#ifdef __cplusplus
}
#endif

#endif
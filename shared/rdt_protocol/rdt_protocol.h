/**
 * @file     rdt_protocol.h
 * @brief    Shared Rover Data Transport protocol definitions.
 */

#ifndef RDT_PROTOCOL_H
#define RDT_PROTOCOL_H

#include <stdbool.h>
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


/* ------------------ RDT Protocol APIs ------------------ */

/** Result of an RDT protocol operation. */
typedef enum {
    RDT_RESULT_OK = 0,
    RDT_RESULT_IN_PROGRESS,
    RDT_RESULT_FRAME_READY,
    RDT_RESULT_CRC_ERROR,
    RDT_RESULT_INVALID_FRAME,
    RDT_RESULT_BUFFER_TOO_SMALL
} RDT_Result_t;

/** Decoded logical RDT frame before application processing. */
typedef struct {
    uint8_t type;
    uint8_t length;
    uint8_t payload[255];
    uint8_t crc;
} RDT_Frame_t;

typedef enum {
    RDT_PARSER_WAIT_START,
    RDT_PARSER_READ_TYPE,
    RDT_PARSER_READ_LENGTH,
    RDT_PARSER_READ_PAYLOAD,
    RDT_PARSER_READ_CRC
} RDT_ParserState_t;

/** Stateful parser for bytes received from the UART stream. */
typedef struct {
    RDT_ParserState_t state;
    bool escaped;
    uint16_t body_length;
    uint8_t body[258];
} RDT_Parser_t;

/**
 * @brief Calculate the CRC-8 for a byte sequence.
 */
uint8_t RDT_CRC8_Calculate(const uint8_t *data, uint16_t length);

/**
 * @brief Validate a CRC-8 value against a byte sequence.
 */
bool RDT_CRC8_Validate(
    const uint8_t *data,
    uint16_t length,
    uint8_t expected_crc
);

/**
 * @brief Encode and escape a logical frame for UART transmission.
 */
RDT_Result_t RDT_Frame_Encode(
    const RDT_Frame_t *frame,
    uint8_t *output,
    uint16_t output_capacity,
    uint16_t *output_length
);

/**
 * @brief Initialize a UART stream parser.
 */
void RDT_Parser_Init(RDT_Parser_t *parser);

/**
 * @brief Submit one received UART byte to the stream parser.
 */
RDT_Result_t RDT_Parser_PushByte(
    RDT_Parser_t *parser,
    uint8_t received_byte,
    RDT_Frame_t *frame
);

/**
 * @brief Decode a status payload into battery voltage and rover speed.
 */
RDT_Result_t RDT_Status_Decode(
    const uint8_t *payload,
    uint8_t payload_length,
    float *battery_voltage,
    float *speed_mps
);

/**
 * @brief Encode battery voltage and rover speed into a status payload.
 */
RDT_Result_t RDT_Status_Encode(
    float battery_voltage,
    float speed_mps,
    uint8_t *payload,
    uint8_t payload_capacity
);

/**
 * @brief Check whether a control payload value is defined by RDT.
 */
bool RDT_Control_IsValid(uint8_t control);

/* ---------------------------------------------------- */

#ifdef __cplusplus
}
#endif

#endif
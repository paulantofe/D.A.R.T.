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


/* ------------------ RDT Protocol Types ------------------ */

/**
 * @brief Result of an RDT protocol operation.
 */
typedef enum {
    RDT_RESULT_OK = 0,
    RDT_RESULT_IN_PROGRESS,
    RDT_RESULT_FRAME_READY,
    RDT_RESULT_CRC_ERROR,
    RDT_RESULT_INVALID_FRAME,
    RDT_RESULT_BUFFER_TOO_SMALL
} RDT_Result_t;

/**
 * @brief Decoded logical RDT frame before application processing.
 *
 * The payload and CRC fields contain unescaped logical bytes. Escaping is
 * applied only to the serialized representation produced by
 * RDT_Frame_Encode().
 */
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

/**
 * @brief Stateful parser for bytes received from the UART stream.
 *
 * A parser instance must be initialized with RDT_Parser_Init() before bytes
 * are submitted with RDT_Parser_PushByte().
 */
typedef struct {
    RDT_ParserState_t state;
    bool escaped;
    uint16_t body_length;
    uint8_t body[258];
} RDT_Parser_t;

/* -------------------------------------------------------- */


/* ------------------ RDT Protocol APIs ------------------ */

/**
 * @brief Calculate the RDT CRC-8 for a byte sequence.
 *
 * The calculation uses the RDT polynomial and processes the input bytes
 * without escaping. For a frame, pass Type, Length, and Payload only.
 *
 * @param[in] data   Bytes to process.
 * @param[in] length Number of bytes in data.
 * @return The calculated CRC-8 value.
 */
uint8_t RDT_CRC8_Calculate(const uint8_t *data, uint16_t length);

/**
 * @brief Validate a CRC-8 value against a byte sequence.
 *
 * @param[in] data         Bytes over which the CRC was calculated.
 * @param[in] length       Number of bytes in data.
 * @param[in] expected_crc CRC value received with the frame.
 * @return true when the calculated CRC equals expected_crc.
 */
bool RDT_CRC8_Validate(
    const uint8_t *data,
    uint16_t length,
    uint8_t expected_crc
);

/**
 * @brief Encode and escape a logical frame for UART transmission.
 *
 * The frame is validated, its CRC is recalculated and stored in frame->crc,
 * and its serialized form is written as Start, escaped body, and escaped CRC.
 * The payload length is the unescaped logical payload length.
 *
 * @param[in,out] frame           Logical frame to encode.
 * @param[out]    output          Destination buffer for serialized bytes.
 * @param[in]     output_capacity Size of output in bytes.
 * @param[out]    output_length   Number of serialized bytes written.
 * @return RDT_RESULT_OK on success, or an error result.
 */
RDT_Result_t RDT_Frame_Encode(
    RDT_Frame_t *frame,
    uint8_t *output,
    uint16_t output_capacity,
    uint16_t *output_length
);

/**
 * @brief Initialize a UART stream parser.
 *
 * @param[out] parser Parser state to initialize.
 */
void RDT_Parser_Init(RDT_Parser_t *parser);

/**
 * @brief Submit one received UART byte to the stream parser.
 *
 * One byte is consumed per call. The parser removes escaping, validates the
 * frame layout and CRC, and copies a completed logical frame to frame.
 *
 * @param[in,out] parser Parser state.
 * @param[in]     received_byte Byte received from UART.
 * @param[out]    frame         Completed frame when the result is
 *                              RDT_RESULT_FRAME_READY.
 * @return The parser state/result after consuming the byte.
 */
RDT_Result_t RDT_Parser_PushByte(
    RDT_Parser_t *parser,
    uint8_t received_byte,
    RDT_Frame_t *frame
);

/**
 * @brief Decode a status payload into battery voltage and rover speed.
 *
 * The payload must contain two four-byte ASCII values in the form a.bc,
 * without a separator.
 *
 * @param[in]  payload         Status payload.
 * @param[in]  payload_length  Number of bytes in payload.
 * @param[out] battery_voltage Decoded battery voltage.
 * @param[out] speed_mps       Decoded rover speed in metres per second.
 * @return RDT_RESULT_OK on success, or RDT_RESULT_INVALID_FRAME.
 */
RDT_Result_t RDT_Status_Decode(
    const uint8_t *payload,
    uint8_t payload_length,
    float *battery_voltage,
    float *speed_mps
);

/**
 * @brief Encode battery voltage and rover speed into a status payload.
 *
 * Values are rounded to two decimal places and must fit the wire format
 * 0.00 through 9.99.
 *
 * @param[in]  battery_voltage Battery voltage to encode.
 * @param[in]  speed_mps       Rover speed in metres per second.
 * @param[out] payload         Destination status payload.
 * @param[in]  payload_capacity Size of payload in bytes.
 * @return RDT_RESULT_OK on success, or RDT_RESULT_INVALID_FRAME.
 */
RDT_Result_t RDT_Status_Encode(
    float battery_voltage,
    float speed_mps,
    uint8_t *payload,
    uint8_t payload_capacity
);

/**
 * @brief Check whether a control payload value is defined by RDT.
 *
 * @param[in] control Control payload byte.
 * @return true when control is one of the defined RDT control values.
 */
bool RDT_Control_IsValid(uint8_t control);

/* ---------------------------------------------------- */

#ifdef __cplusplus
}
#endif

#endif
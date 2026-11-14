#include "rdt_protocol.h"

static void RDT_Parser_Reset(RDT_Parser_t *parser) {
    parser->state = RDT_PARSER_WAIT_START;
    parser->escaped = false;
    parser->body_length = 0U;
}

static RDT_Result_t RDT_Parser_ConsumeByte(
    RDT_Parser_t *parser,
    uint8_t byte,
    RDT_Frame_t *frame
) {
    switch (parser->state) {
        case RDT_PARSER_READ_TYPE:
            parser->body[0] = byte;
            parser->body_length = 1U;
            parser->state = RDT_PARSER_READ_LENGTH;
            return RDT_RESULT_IN_PROGRESS;

        case RDT_PARSER_READ_LENGTH:
            parser->body[1] = byte;
            parser->body_length = 2U;
            parser->state = byte == 0U
                                ? RDT_PARSER_READ_CRC
                                : RDT_PARSER_READ_PAYLOAD;
            return RDT_RESULT_IN_PROGRESS;

        case RDT_PARSER_READ_PAYLOAD:
            parser->body[parser->body_length++] = byte;

            if (parser->body_length >=
                (uint16_t)parser->body[1] + 2U) {
                parser->state = RDT_PARSER_READ_CRC;
            }

            return RDT_RESULT_IN_PROGRESS;

        case RDT_PARSER_READ_CRC: {
            const uint8_t payload_length = parser->body[1];
            const uint16_t crc_data_length =
                (uint16_t)payload_length + 2U;

            if (!RDT_CRC8_Validate(
                    parser->body,
                    crc_data_length,
                    byte)) {
                RDT_Parser_Reset(parser);
                return RDT_RESULT_CRC_ERROR;
            }

            frame->type = parser->body[0];
            frame->length = payload_length;
            frame->crc = byte;

            for (uint8_t i = 0U; i < payload_length; ++i) {
                frame->payload[i] = parser->body[i + 2U];
            }

            RDT_Parser_Reset(parser);
            return RDT_RESULT_FRAME_READY;
        }

        case RDT_PARSER_WAIT_START:
        default:
            return RDT_RESULT_INVALID_FRAME;
    }
}

uint8_t RDT_CRC8_Calculate(const uint8_t *data, uint16_t length) {
    uint8_t crc = 0x00U;

    for (uint16_t byte = 0; byte < length; ++byte) {
        crc ^= data[byte];

        for (uint8_t bit = 0; bit < 8U; ++bit) {
            if (crc & 0x80U) {
                crc = (uint8_t) ((crc << 1U) ^ RDT_CRC8_POLYNOMIAL);
            } else {
                crc = (uint8_t) (crc << 1U);
            }
        }
    }

    return crc;
}

bool RDT_CRC8_Validate(
    const uint8_t *data,
    uint16_t length,
    uint8_t expected_crc
) {
    uint8_t calculated_crc = RDT_CRC8_Calculate(data, length);

    return calculated_crc == expected_crc;
}

static bool RDT_AppendByte(
    uint8_t value,
    uint8_t *output,
    uint16_t capacity,
    uint16_t *position
) {
    const bool must_escape =
        value == RDT_START_BYTE || value == RDT_ESCAPE_BYTE;

    const uint16_t required = must_escape ? 2U : 1U;

    if (*position > capacity || capacity - *position < required) {
        return false;
    }

    if (must_escape) {
        output[(*position)++] = RDT_ESCAPE_BYTE;
    }

    output[(*position)++] = value;
    return true;
}

RDT_Result_t RDT_Frame_Encode(
    const RDT_Frame_t *frame,
    uint8_t *output,
    uint16_t output_capacity,
    uint16_t *output_length
) {
    if (frame == NULL || output == NULL || output_length == NULL) {
        return RDT_RESULT_INVALID_FRAME;
    }

    if (output_capacity == 0U) {
        return RDT_RESULT_BUFFER_TOO_SMALL;
    }

    const uint16_t crc_data_length = (uint16_t)frame->length + 2U;

    uint8_t crc_data[257];
    crc_data[0] = frame->type;
    crc_data[1] = frame->length;

    for (uint8_t i = 0; i < frame->length; ++i) {
        crc_data[i + 2U] = frame->payload[i];
    }

    const uint8_t crc = RDT_CRC8_Calculate(crc_data, crc_data_length);

    uint16_t position = 0U;
    output[position++] = RDT_START_BYTE;

    if (!RDT_AppendByte(frame->type, output, output_capacity, &position) ||
        !RDT_AppendByte(frame->length, output, output_capacity, &position)) {
        return RDT_RESULT_BUFFER_TOO_SMALL;
    }

    for (uint8_t i = 0; i < frame->length; ++i) {
        if (!RDT_AppendByte(
                frame->payload[i],
                output,
                output_capacity,
                &position)) {
            return RDT_RESULT_BUFFER_TOO_SMALL;
        }
    }

    if (!RDT_AppendByte(crc, output, output_capacity, &position)) {
        return RDT_RESULT_BUFFER_TOO_SMALL;
    }

    *output_length = position;
    return RDT_RESULT_OK;
}


void RDT_Parser_Init(RDT_Parser_t *parser) {
    if (parser != NULL) {
        RDT_Parser_Reset(parser);
    }
}


RDT_Result_t RDT_Parser_PushByte(
    RDT_Parser_t *parser,
    uint8_t received_byte,
    RDT_Frame_t *frame
) {
    if (parser == NULL || frame == NULL) {
        return RDT_RESULT_INVALID_FRAME;
    }

    if (parser->state == RDT_PARSER_WAIT_START) {
        if (received_byte == RDT_START_BYTE) {
            parser->state = RDT_PARSER_READ_TYPE;
            parser->escaped = false;
            parser->body_length = 0U;
        }

        return RDT_RESULT_IN_PROGRESS;
    }

    if (!parser->escaped && received_byte == RDT_START_BYTE) {
        RDT_Parser_Reset(parser);
        parser->state = RDT_PARSER_READ_TYPE;
        return RDT_RESULT_IN_PROGRESS;
    }

    if (parser->escaped) {
        parser->escaped = false;
    } else if (received_byte == RDT_ESCAPE_BYTE) {
        parser->escaped = true;
        return RDT_RESULT_IN_PROGRESS;
    }

    return RDT_Parser_ConsumeByte(parser, received_byte, frame);
}

static bool RDT_ParseFloat(const uint8_t *text, float *value) {
    if (text[0] < '0' || text[0] > '9' ||
        text[1] != '.' ||
        text[2] < '0' || text[2] > '9' ||
        text[3] < '0' || text[3] > '9') {
        return false;
    }

    *value = (float) (text[0] - '0') + 
             (float) (text[2] - '0') / 10.0f +
             (float) (text[3] - '0') / 100.0f;

    return true;
}

RDT_Result_t RDT_Status_Decode(
    const uint8_t *payload,
    uint8_t payload_length,
    float *battery_voltage,
    float *speed_mps
) {
    if (payload == NULL || payload_length == 0 || payload_length != RDT_STATUS_LENGTH ||
        battery_voltage == NULL || speed_mps == NULL) {
        return RDT_RESULT_INVALID_FRAME;
    }

    bool valid_battery = RDT_ParseFloat(&payload[0], battery_voltage);
    bool valid_speed   = RDT_ParseFloat(&payload[4], speed_mps);

    if (!valid_battery || !valid_speed) {
        return RDT_RESULT_INVALID_FRAME;
    }

    return RDT_RESULT_OK;
}

static bool RDT_ParseText(float value, uint8_t *text) {
    if (value < 0.0f || value > 9.99f) {
        return false;
    }

    const uint16_t scaled =
        (uint16_t)(value * 100.0f + 0.5f);

    text[0] = (uint8_t)(scaled / 100U) + '0';
    text[1] = '.';
    text[2] = (uint8_t)((scaled / 10U) % 10U) + '0';
    text[3] = (uint8_t)(scaled % 10U) + '0';

    return true;
}

RDT_Result_t RDT_Status_Encode(
    float battery_voltage,
    float speed_mps,
    uint8_t *payload,
    uint8_t payload_capacity
) {
    if (payload == NULL || payload_capacity < RDT_STATUS_LENGTH) {
        return RDT_RESULT_INVALID_FRAME;
    }

    bool valid_battery = RDT_ParseText(battery_voltage, &payload[0]);
    bool valid_speed   = RDT_ParseText(speed_mps, &payload[4]);

    if (!valid_battery || !valid_speed) {
        return RDT_RESULT_INVALID_FRAME;
    }

    return RDT_RESULT_OK;
}

bool RDT_Control_IsValid(uint8_t control) {
    switch (control) {
        case RDT_CONTROL_FORWARD:
        case RDT_CONTROL_FORWARD_LEFT:
        case RDT_CONTROL_LEFT:
        case RDT_CONTROL_BACKWARD_LEFT:
        case RDT_CONTROL_BACKWARD:
        case RDT_CONTROL_BACKWARD_RIGHT:
        case RDT_CONTROL_RIGHT:
        case RDT_CONTROL_FORWARD_RIGHT:
        case RDT_CONTROL_STOP:
        case RDT_CONTROL_SYSTEM_FAULT:
        case RDT_CONTROL_FAILSAFE:
            return true;

        default:
            return false;
    }
}

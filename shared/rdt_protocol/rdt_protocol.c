#include "rdt_protocol.h"

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

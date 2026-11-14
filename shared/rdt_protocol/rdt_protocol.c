#include "rdt_protocol.h"

uint8_t RDT_CRC8_Calculate(const uint8_t *data, uint8_t length) {
    uint8_t crc = 0x00U;

    for (uint8_t byte = 0; byte < length; ++byte) {
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
#include "rdt_link.h"
#include "globals.h"

static RDT_Parser_t rdt_parser;

void RDT_Link_Init(void) {
    RDT_Parser_Init(&rdt_parser);
}

RDT_Result_t RDT_SendFrame(const uint8_t type, const uint8_t *payload, uint8_t payload_length) {
    if (payload == nullptr && payload_length > 0U) {
        return RDT_RESULT_INVALID_FRAME;
    }

    RDT_Frame_t tx_frame = { 0 };

    tx_frame.type = type;
    for (uint8_t i = 0; i < payload_length; ++i) {
        tx_frame.payload[i] = payload[i];
    }
    tx_frame.length = payload_length;

    uint8_t tx_buffer[520];
    uint16_t tx_length = 0U;

    const RDT_Result_t result =
        RDT_Frame_Encode(
            &tx_frame,
            tx_buffer,
            sizeof(tx_buffer),
            &tx_length
        );

    if (result != RDT_RESULT_OK) {
        return result;
    }

    Serial1.write(tx_buffer, tx_length);
    return RDT_RESULT_OK;
}

void RDT_Handle_IncomingByte(uint8_t received) {
    RDT_Frame_t frame;

    const RDT_Result_t result =
        RDT_Parser_PushByte(&rdt_parser, received, &frame);

    if (result != RDT_RESULT_FRAME_READY) {
        return;
    }

    switch (frame.type) {
        case RDT_HANDSHAKE_TYPE:
            RDT_Handle_Handshake(&frame);
            break;

        case RDT_STATUS_TYPE:
            RDT_Handle_Status(&frame);
            break;

        case RDT_CONTROL_TYPE:
            /* N/A for ESP32 */
            break;

        default:
            break;
    }
}

static uint8_t req_counter;
static uint32_t last_req;
static uint32_t handshake_started;
static uint32_t last_status_received;
static bool reset = true;

static void RDT_Reset_Handshake(void) {
    const uint32_t now = millis();

    req_counter = 1;
    last_req = now;
    handshake_started = now;
    reset = false;
    link_state = Link_State_t::LINK_HANDSHAKING;
}

static void RDT_Conclude_Handshake(void) {
    const uint8_t response = RDT_ESP_OK;
    const RDT_Result_t result =
        RDT_SendFrame(
            RDT_HANDSHAKE_TYPE,
            &response,
            RDT_HANDSHAKE_LENGTH
        );

    if (result != RDT_RESULT_OK) {
        link_state = Link_State_t::LINK_OFFLINE;
        reset = true;
        req_counter = 0U;
        return;
    }

    link_state = Link_State_t::LINK_ONLINE;
    last_req = millis();
    last_status_received = last_req;
    reset = false;
}

void RDT_Handle_Handshake(RDT_Frame_t *frame) {
    if (frame->payload[0] != RDT_STM_REQ) {
        return;
    }

    if (reset) {
        RDT_Reset_Handshake();
        return;
    }

    const uint32_t now = millis();

    if (link_state == Link_State_t::LINK_ONLINE) {
        last_req = now;
        return;
    }

    if (now - last_req > RDT_REQUEST_INTERVAL_TIMEOUT_MS) {
        RDT_Reset_Handshake();
        return;
    }

    ++req_counter;
    last_req = now;

    if (req_counter >= RDT_REQUIRED_VALID_REQUESTS) {
        RDT_Conclude_Handshake();
    }
}

void RDT_Link_CheckTimeout(uint32_t now) {
    if (link_state == Link_State_t::LINK_ONLINE) {
        if (now - last_req > RDT_HANDSHAKE_TIMEOUT_MS ||
            now - last_status_received > RDT_ESP_STATUS_TIMEOUT_MS) {
            link_state = Link_State_t::LINK_OFFLINE;
            req_counter = 0U;
            reset = true;
            current_command = RDT_CONTROL_STOP;
        }

        return;
    }

    if (link_state == Link_State_t::LINK_HANDSHAKING &&
        now - handshake_started > RDT_HANDSHAKE_TIMEOUT_MS) {
        link_state = Link_State_t::LINK_OFFLINE;
        req_counter = 0U;
        reset = true;
        current_command = RDT_CONTROL_STOP;
    }
}

void RDT_Handle_Status(RDT_Frame_t *frame) {
    float battery, speed;
    
    const RDT_Result_t decode_result =
        RDT_Status_Decode(
            frame->payload,
            frame->length,
            &battery,
            &speed
        );

    if (decode_result == RDT_RESULT_OK) {
        battery_level = String(battery);
        speed_mps = String(speed);
        last_status_received = millis();
    }
}

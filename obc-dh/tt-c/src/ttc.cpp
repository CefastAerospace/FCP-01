#include "ttc.h"
#include "conops.h"

TTC_Module TTC;

TTC_Module::TTC_Module() {
    _telemetry.last_rssi = -120;
    _telemetry.last_snr = 0.0f;
    _telemetry.rx_packets_count = 0;
    _telemetry.tx_packets_count = 0;
    _telemetry.rx_errors_count = 0;
    _telemetry.is_transmitting = false;
}

uint16_t TTC_Module::CalculateCRC16(const uint8_t *data, size_t length) {
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < length; i++) {
        crc ^= (uint16_t)data[i] << 8;
        for (uint8_t bit = 0; bit < 8; bit++) {
            if (crc & 0x8000) {
                crc = (crc << 1) ^ 0x1021;
            } else {
                crc <<= 1;
            }
        }
    }
    return crc;
}

SubsystemStatus_t TTC_Module::Init() {
    pinMode(33, OUTPUT); // Pino do Burn Wire
    digitalWrite(33, LOW);

    _telemetry.rx_packets_count = 0;
    _telemetry.tx_packets_count = 0;
    _telemetry.rx_errors_count = 0;
    _telemetry.last_rssi = -120;

    return SUBSYS_OK;
}

void TTC_Module::TaskUpdate() {
    // Loop de escuta contínua executado pela Task do FreeRTOS no main.cpp
    /*
    if (LoRa_HasPacket()) {
        TelecommandPacket_t rx_packet;
        LoRa_ReadPacket((uint8_t*)&rx_packet, sizeof(TelecommandPacket_t));
        _telemetry.last_rssi = LoRa_GetRSSI();
        ProcessPacket(rx_packet);
    }
    */
}

SubsystemStatus_t TTC_Module::GetTelemetry(uint8_t *buffer, size_t max_len, size_t *out_len) {
    if (buffer == NULL || out_len == NULL) return SUBSYS_ERR_PARAM_INVALID;
    if (max_len < sizeof(TTC_Telemetry_t)) return SUBSYS_ERR_PARAM_INVALID;

    memcpy(buffer, &_telemetry, sizeof(TTC_Telemetry_t));
    *out_len = sizeof(TTC_Telemetry_t);
    return SUBSYS_OK;
}

SubsystemStatus_t TTC_Module::HandleCommand(const SubsystemCommand_t &cmd) {
    // Adapter para redirecionar chamadas genéricas
    return SUBSYS_OK;
}

bool TTC_Module::HealthCheck() {
    // Retorna true se o rádio estiver operacional
    return true;
}

void TTC_Module::SendACK(uint16_t sequence_id, uint8_t command_id, uint8_t status_code) {
    ACKPacket_t ack;
    ack.header = 0xAA55;
    ack.sequence_id = sequence_id;
    ack.command_id = command_id;
    ack.status_code = status_code;
    ack.checksum = CalculateCRC16((const uint8_t*)&ack, sizeof(ACKPacket_t) - sizeof(uint16_t));

    _telemetry.tx_packets_count++;
}

void TTC_Module::SendTelemetryPacket() {
    _telemetry.tx_packets_count++;
}

void TTC_Module::ProcessPacket(const TelecommandPacket_t &pkt) {
    uint16_t computed_crc = CalculateCRC16((const uint8_t*)&pkt, sizeof(TelecommandPacket_t) - sizeof(uint16_t));
    if (computed_crc != pkt.checksum) {
        _telemetry.rx_errors_count++;
        SendACK(pkt.sequence_id, pkt.command_id, ERR_CRC);
        return;
    }

    _telemetry.rx_packets_count++;

    switch (pkt.command_id) {
        case CMD_ENTER_SAFE: {
            if (System_SetState(STATE_SAFE)) {
                // SD_Log("CONOPS", "CMD", LOG_EVENT, "SAFE MODE ativado via GS");
                SendACK(pkt.sequence_id, pkt.command_id, ACK_OK);
            } else {
                SendACK(pkt.sequence_id, pkt.command_id, ERR_INVALID_CMD);
            }
            break;
        }

        case CMD_EXIT_SAFE: {
            uint16_t confirm_key = *(uint16_t*)&pkt.arguments[0];
            if (confirm_key != 0xA55A) {
                SendACK(pkt.sequence_id, pkt.command_id, ERR_INVALID_PARAM);
                return;
            }

            if (System_SetState(STATE_MISSION)) {
                // SD_Log("CONOPS", "CMD", LOG_EVENT, "Retorno para MISSION MODE via GS");
                SendACK(pkt.sequence_id, pkt.command_id, ACK_OK);
            } else {
                SendACK(pkt.sequence_id, pkt.command_id, ERR_INVALID_CMD);
            }
            break;
        }

        case CMD_SET_MODE: {
            SystemState_t targetState = (SystemState_t)pkt.arguments[0];
            if (System_SetState(targetState)) {
                SendACK(pkt.sequence_id, pkt.command_id, ACK_OK);
            } else {
                SendACK(pkt.sequence_id, pkt.command_id, ERR_INVALID_PARAM);
            }
            break;
        }

        case CMD_DEPLOY_ANTENNA: {
            if (System_GetState() != STATE_MISSION) {
                Serial.println("[TT&C] Bloqueado: Burn wire acionado fora do modo MISSION!");
                SendACK(pkt.sequence_id, pkt.command_id, ERR_INVALID_CMD);
                return;
            }

            uint8_t burn_time = pkt.arguments[0];
            if (burn_time > 10) {
                SendACK(pkt.sequence_id, pkt.command_id, ERR_INVALID_PARAM);
                return;
            }

            digitalWrite(33, HIGH);
            vTaskDelay(pdMS_TO_TICKS(burn_time * 1000));
            digitalWrite(33, LOW);

            SendACK(pkt.sequence_id, pkt.command_id, ACK_OK);
            break;
        }

        case CMD_PING: {
            SendACK(pkt.sequence_id, pkt.command_id, ACK_OK);
            break;
        }

        case CMD_REQUEST_TELEMETRY: {
            SendTelemetryPacket();
            SendACK(pkt.sequence_id, pkt.command_id, ACK_OK);
            break;
        }

        default: {
            SendACK(pkt.sequence_id, pkt.command_id, ERR_UNKNOWN_CMD);
            break;
        }
    }
}

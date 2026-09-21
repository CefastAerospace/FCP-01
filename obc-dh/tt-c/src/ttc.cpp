#include "ttc.h"
#include "../../src/conops.h"
#include "../../eps-tc/src/eps.h"
#include "../../adcs/adcs.h"

// Seleção do rádio: real (SX1276) ou mock (testes sem hardware)
#ifdef TTC_USE_MOCK
#include "../../src/drivers/lora_mock.h"
static LoRaMock g_lora_mock;
#else
static LoRaDriver g_lora_driver;
#endif

TTC_Module TTC;

static constexpr BaseType_t TX_QUEUE_LENGTH = 10;

#ifdef TTC_USE_MOCK
LoRaMock* TTC_GetMock() {
    return &g_lora_mock;
}
#endif

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
    pinMode(33, OUTPUT);
    digitalWrite(33, LOW);

#ifdef TTC_USE_MOCK
    _radio = &g_lora_mock;
#else
    _radio = &g_lora_driver;
#endif

    if (!_radio->Init()) {
        Serial.println("[TT&C] ERRO: falha ao inicializar radio LoRa!");
        return SUBSYS_ERR_INIT_FAILED;
    }

    _txQueue = xQueueCreate(TX_QUEUE_LENGTH, sizeof(LoRaPacket_t));
    if (_txQueue == NULL) {
        Serial.println("[TT&C] ERRO: falha ao criar fila TX!");
        return SUBSYS_ERR_INIT_FAILED;
    }

    _sequence_id = 0;
    _beacon_enabled = false;
    _beacon_interval_ms = 10000;
    _last_beacon_ms = 0;

    _telemetry.rx_packets_count = 0;
    _telemetry.tx_packets_count = 0;
    _telemetry.rx_errors_count = 0;
    _telemetry.last_rssi = -120;
    _telemetry.last_snr = 0.0f;
    _telemetry.is_transmitting = false;

    Serial.println("[TT&C] Modulo inicializado com sucesso.");
    return SUBSYS_OK;
}

void TTC_Module::TaskUpdate() {
    if (_radio == NULL) return;

    uint8_t rx_buffer[LORA_MAX_PACKET_SIZE];
    int rx_len = _radio->Receive(rx_buffer, sizeof(rx_buffer));

    if (rx_len >= (int)sizeof(TelecommandPacket_t)) {
        _telemetry.last_rssi = _radio->GetRSSI();
        _telemetry.last_snr = _radio->GetSNR();

        TelecommandPacket_t rx_packet;
        memcpy(&rx_packet, rx_buffer, sizeof(TelecommandPacket_t));
        ProcessPacket(rx_packet);
    }

    // Beacon periódico: transmite telemetria no intervalo configurado
    if (_beacon_enabled && (millis() - _last_beacon_ms >= _beacon_interval_ms)) {
        _last_beacon_ms = millis();
        SendTelemetryPacket();
    }

    // Conclusão da queima do burn wire (não-bloqueante; subtração com sinal tolera overflow do millis)
    if (_burn_active && (int32_t)(millis() - _burn_end_ms) >= 0) {
        digitalWrite(33, LOW);
        _burn_active = false;
        Serial.println("[TT&C] Queima do burn wire concluída.");
    }

    FlushTxQueue();
    // Sem vTaskDelay aqui: a cadência é dada pelo vTaskDelayUntil(100 ms) do TaskTTC no main.cpp
}

SubsystemStatus_t TTC_Module::GetTelemetry(uint8_t *buffer, size_t max_len, size_t *out_len) {
    if (buffer == NULL || out_len == NULL) return SUBSYS_ERR_PARAM_INVALID;
    if (max_len < sizeof(TTC_Telemetry_t)) return SUBSYS_ERR_PARAM_INVALID;

    memcpy(buffer, &_telemetry, sizeof(TTC_Telemetry_t));
    *out_len = sizeof(TTC_Telemetry_t);
    return SUBSYS_OK;
}

SubsystemStatus_t TTC_Module::HandleCommand(const SubsystemCommand_t &cmd) {
    switch (cmd.command_id) {
        case TTC_CMD_TX_TELEMETRY: {
            SendTelemetryPacket();
            return SUBSYS_OK;
        }

        case TTC_CMD_BEACON_ON: {
            _beacon_enabled = true;
            _last_beacon_ms = millis();
            return SUBSYS_OK;
        }

        case TTC_CMD_BEACON_OFF: {
            _beacon_enabled = false;
            return SUBSYS_OK;
        }

        case TTC_CMD_SET_BEACON_INT: {
            if (cmd.payload_len < sizeof(uint32_t)) {
                return SUBSYS_ERR_PARAM_INVALID;
            }
            uint32_t interval_ms;
            memcpy(&interval_ms, cmd.payload, sizeof(uint32_t));
            if (interval_ms < 1000) {
                return SUBSYS_ERR_PARAM_INVALID;
            }
            _beacon_interval_ms = interval_ms;
            return SUBSYS_OK;
        }

        case TTC_CMD_TX_RAW: {
            if (cmd.payload_len == 0) return SUBSYS_ERR_PARAM_INVALID;
            return QueueTxPacket(cmd.payload, cmd.payload_len) ? SUBSYS_OK : SUBSYS_ERR_TIMEOUT;
        }

        case TTC_CMD_RESET_STATS: {
            _telemetry.rx_packets_count = 0;
            _telemetry.tx_packets_count = 0;
            _telemetry.rx_errors_count = 0;
            return SUBSYS_OK;
        }

        default:
            return SUBSYS_ERR_PARAM_INVALID;
    }
}

bool TTC_Module::QueueTxPacket(const uint8_t* data, size_t len) {
    if (data == NULL || len == 0 || len > LORA_MAX_PACKET_SIZE) return false;
    if (_txQueue == NULL) return false;

    LoRaPacket_t tx_pkt;
    tx_pkt.len = (uint8_t)len;
    memcpy(tx_pkt.data, data, len);

    if (xQueueSend(_txQueue, &tx_pkt, pdMS_TO_TICKS(100)) == pdTRUE) {
        _telemetry.tx_packets_count++;
        return true;
    }
    return false;
}

bool TTC_Module::HealthCheck() {
    // Retorna true se o rádio estiver operacional
    return (_radio != NULL) && _radio->IsAvailable();
}

void TTC_Module::FlushTxQueue() {
    if (_radio == NULL || _txQueue == NULL) return;

    LoRaPacket_t tx_pkt;
    while (xQueueReceive(_txQueue, &tx_pkt, 0) == pdTRUE) {
        _telemetry.is_transmitting = true;
        Serial.printf("[TT&C] TX %u bytes no ar\n", tx_pkt.len);
        _radio->Send(tx_pkt.data, tx_pkt.len);
        _telemetry.is_transmitting = false;
    }
}

void TTC_Module::SendACK(uint16_t sequence_id, uint8_t command_id, uint8_t status_code) {
    ACKPacket_t ack;
    ack.header = 0xAA55;
    ack.sequence_id = sequence_id;
    ack.command_id = command_id;
    ack.status_code = status_code;
    ack.checksum = CalculateCRC16((const uint8_t*)&ack, sizeof(ACKPacket_t) - sizeof(uint16_t));

    LoRaPacket_t tx_pkt;
    tx_pkt.len = sizeof(ACKPacket_t);
    memcpy(tx_pkt.data, &ack, sizeof(ACKPacket_t));

    if (xQueueSend(_txQueue, &tx_pkt, pdMS_TO_TICKS(100)) == pdTRUE) {
        _telemetry.tx_packets_count++;
    } else {
        Serial.println("[TT&C] Fila TX cheia, ACK descartado!");
    }
}

void TTC_Module::SendTelemetryPacket() {
    TelemetryPacket_t tm;
    tm.header = TM_HEADER;
    tm.sequence_id = _sequence_id++;
    tm.packet_type = TM_TYPE_TELEMETRY;
    tm.system_status = (uint8_t)System_GetState();
    tm.timestamp = (uint32_t)((_time_offset_ms + (int64_t)millis()) / 1000LL);

    // --- EPS ---
    uint8_t eps_buf[64];
    size_t eps_len = 0;
    if (g_eps.GetTelemetry(eps_buf, sizeof(eps_buf), &eps_len) == SUBSYS_OK && eps_len >= sizeof(EPS_Telemetry_t)) {
        EPS_Telemetry_t *eps = (EPS_Telemetry_t*)eps_buf;
        tm.vbat = eps->bus_voltage;
        tm.ibat = eps->crnt_mA;
        tm.temp_obc = eps->temp_C;
    } else {
        tm.vbat = 0.0f;
        tm.ibat = 0.0f;
        tm.temp_obc = 0.0f;
    }

    // --- ADCS ---
    uint8_t adcs_buf[64];
    size_t adcs_len = 0;
    if (ADCS.GetTelemetry(adcs_buf, sizeof(adcs_buf), &adcs_len) == SUBSYS_OK && adcs_len >= sizeof(ADCS_Telemetry_t)) {
        ADCS_Telemetry_t *adcs = (ADCS_Telemetry_t*)adcs_buf;
        tm.rpm = adcs->current_rpm;
        tm.target_rpm = adcs->target_rpm;
    } else {
        tm.rpm = 0.0f;
        tm.target_rpm = 0.0f;
    }

    // --- Link TT&C ---
    tm.last_rssi = _telemetry.last_rssi;
    tm.last_snr = _telemetry.last_snr;
    tm.rx_packets_count = _telemetry.rx_packets_count;
    tm.tx_packets_count = _telemetry.tx_packets_count;
    tm.rx_errors_count = _telemetry.rx_errors_count;

    tm.checksum = CalculateCRC16((const uint8_t*)&tm, sizeof(TelemetryPacket_t) - sizeof(uint16_t));

    LoRaPacket_t tx_pkt;
    tx_pkt.len = sizeof(TelemetryPacket_t);
    memcpy(tx_pkt.data, &tm, sizeof(TelemetryPacket_t));

    if (xQueueSend(_txQueue, &tx_pkt, pdMS_TO_TICKS(100)) == pdTRUE) {
        _telemetry.tx_packets_count++;
    } else {
        Serial.println("[TT&C] Fila TX cheia, telemetria descartada!");
    }
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
            System_SetState(STATE_SAFE);
            // SD_Log("CONOPS", "CMD", LOG_EVENT, "SAFE MODE ativado via GS");
            SendACK(pkt.sequence_id, pkt.command_id, ACK_OK);
            break;
        }

        case CMD_EXIT_SAFE: {
            uint16_t confirm_key = *(uint16_t*)&pkt.arguments[0];
            if (confirm_key != 0xA55A) {
                SendACK(pkt.sequence_id, pkt.command_id, ERR_INVALID_PARAM);
                return;
            }

            System_SetState(STATE_MISSION);
            // SD_Log("CONOPS", "CMD", LOG_EVENT, "Retorno para MISSION MODE via GS");
            SendACK(pkt.sequence_id, pkt.command_id, ACK_OK);
            break;
        }

        case CMD_SET_MODE: {
            SystemState_t targetState = (SystemState_t)pkt.arguments[0];
            if (targetState > STATE_SAFE) {
                SendACK(pkt.sequence_id, pkt.command_id, ERR_INVALID_PARAM);
                return;
            }
            System_SetState(targetState);
            SendACK(pkt.sequence_id, pkt.command_id, ACK_OK);
            break;
        }

        case CMD_DEPLOY_ANTENNA: {
            if (System_GetState() != STATE_MISSION) {
                Serial.println("[TT&C] Bloqueado: Burn wire acionado fora do modo MISSION!");
                SendACK(pkt.sequence_id, pkt.command_id, ERR_INVALID_CMD);
                return;
            }

            if (_burn_active) {
                Serial.println("[TT&C] Queima já em andamento, comando rejeitado!");
                SendACK(pkt.sequence_id, pkt.command_id, ERR_STATE_REJECTED);
                return;
            }

            uint8_t burn_time = pkt.arguments[0];
            if (burn_time == 0 || burn_time > 10) {
                SendACK(pkt.sequence_id, pkt.command_id, ERR_INVALID_PARAM);
                return;
            }

            // Queima não-bloqueante: liga agora, TaskUpdate desliga ao expirar
            digitalWrite(33, HIGH);
            _burn_end_ms = millis() + (uint32_t)burn_time * 1000UL;
            _burn_active = true;

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

        case CMD_START_MISSION: {
            if (System_GetState() == STATE_SAFE) {
                System_SetState(STATE_MISSION);
                SendACK(pkt.sequence_id, pkt.command_id, ACK_OK);
            } else {
                SendACK(pkt.sequence_id, pkt.command_id, ERR_STATE_REJECTED);
            }
            break;
        }

        case CMD_STOP_MISSION: {
            if (System_GetState() == STATE_MISSION) {
                System_SetState(STATE_SAFE);
                SendACK(pkt.sequence_id, pkt.command_id, ACK_OK);
            } else {
                SendACK(pkt.sequence_id, pkt.command_id, ERR_INVALID_CMD);
            }
            break;
        }

        case CMD_GET_STATUS: {
            SendTelemetryPacket();
            SendACK(pkt.sequence_id, pkt.command_id, ACK_OK);
            break;
        }

        case CMD_SET_TIME: {
            uint32_t unix_time;
            memcpy(&unix_time, pkt.arguments, sizeof(uint32_t));
            _time_offset_ms = (int64_t)unix_time * 1000LL - (int64_t)millis();
            SendACK(pkt.sequence_id, pkt.command_id, ACK_OK);
            break;
        }

        case CMD_ADCS_START:
        case CMD_ADCS_STOP: {
            SubsystemCommand_t adcs_cmd;
            adcs_cmd.command_id = pkt.command_id;
            adcs_cmd.payload_len = 0;

            SubsystemStatus_t rc = ADCS.HandleCommand(adcs_cmd);
            SendACK(pkt.sequence_id, pkt.command_id,
                    (rc == SUBSYS_OK) ? ACK_OK : ERR_INVALID_CMD);
            break;
        }

        case CMD_REQUEST_LOG: {
            // ECHO do log via telemetria seria extenso; aqui confirmamos a solicitação.
            SendACK(pkt.sequence_id, pkt.command_id, ACK_OK);
            SendTelemetryPacket();
            break;
        }

        case CMD_RESET_OBC: {
            SendACK(pkt.sequence_id, pkt.command_id, ACK_OK);
            vTaskDelay(pdMS_TO_TICKS(200));
            ESP.restart();
            break;
        }

        default: {
            SendACK(pkt.sequence_id, pkt.command_id, ERR_UNKNOWN_CMD);
            break;
        }
    }
}

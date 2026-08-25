#include "ttc.h"
#include <RTClib.h>
#include "sd_logger.h"

// Referência global do RTC declarada no main.cpp
extern RTC_DS3231 rtc;

// Variáveis internas para telemetria do módulo TT&C
static TTC_Telemetry_t g_ttc_telemetry = {
    .last_rssi = 0,
    .last_snr = 0.0f,
    .rx_packets_count = 0,
    .tx_packets_count = 0,
    .rx_errors_count = 0,
    .is_transmitting = false
};

// --- Funções Auxiliares e Protocolo RF ---

uint16_t TTC_CalculateCRC16(const uint8_t *data, size_t len) {
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (uint8_t j = 0; j < 8; j++) {
            if (crc & 0x0001) crc = (crc >> 1) ^ 0xA001;
            else crc >>= 1;
        }
    }
    return crc;
}

void TTC_SendACK(uint16_t sequence_id, uint8_t command_id, CmdStatus_t status) {
    Serial.printf("[TT&C] ACK | SEQ: %u | CMD: 0x%02X | Status: 0x%02X\n", 
                  sequence_id, command_id, status);
    // TODO: Transmitir pacote ACK/NACK via LoRa SX1276
}

bool TTC_SendTelemetry(const PacketTelemetry_t &packet) {
    g_ttc_telemetry.is_transmitting = true;
    
    // TODO: Chamar o driver do rádio LoRa para envio dos dados
    
    g_ttc_telemetry.tx_packets_count++;
    g_ttc_telemetry.is_transmitting = false;
    return true;
}

// Processador central de pacotes (Dispatcher)
void TTC_ProcessPacket(const TelecommandPacket_t &pkt) {
    // 1. Validação de integridade CRC16
    uint16_t computed_crc = TTC_CalculateCRC16((const uint8_t*)&pkt, sizeof(TelecommandPacket_t) - sizeof(uint16_t));
    if (computed_crc != pkt.checksum) {
        g_ttc_telemetry.rx_errors_count++;
        TTC_SendACK(pkt.sequence_id, pkt.command_id, ERR_CRC);
        return;
    }

    g_ttc_telemetry.rx_packets_count++;

    // 2. Execução por Opcode
    switch (pkt.command_id) {
        case CMD_PING:
            TTC_SendACK(pkt.sequence_id, pkt.command_id, ACK_OK);
            break;

        case CMD_SET_TIME: {
            uint32_t new_time = *(uint32_t*)&pkt.arguments[0];
            
            if (new_time < 1770000000) { // Validação de data coerente
                TTC_SendACK(pkt.sequence_id, pkt.command_id, ERR_INVALID_PARAM);
                return;
            }

            rtc.adjust(DateTime(new_time));
            SD_Log("RTC", "SYNC", LOG_EVENT, "RTC atualizado via GS");
            Serial.printf("[TT&C] RTC sincronizado: %u\n", new_time);
            
            TTC_SendACK(pkt.sequence_id, pkt.command_id, ACK_OK);
            break;
        }

        case CMD_DEPLOY_ANTENNA: {
            uint8_t burn_time = pkt.arguments[0];
            if (burn_time > 10) { // Trava de segurança térmica (max 10s)
                TTC_SendACK(pkt.sequence_id, pkt.command_id, ERR_INVALID_PARAM);
                return;
            }
            
            SD_Log("DEPLOY", "ANTENNA", LOG_EVENT, "Acionamento do Burn Wire");
            digitalWrite(33, HIGH); // PINO BURN
            vTaskDelay(pdMS_TO_TICKS(burn_time * 1000));
            digitalWrite(33, LOW);
            
            TTC_SendACK(pkt.sequence_id, pkt.command_id, ACK_OK);
            break;
        }

        case CMD_RESET_OBC: {
            uint16_t confirm_key = *(uint16_t*)&pkt.arguments[0];
            if (confirm_key != 0xA55A) { // Validação por chave
                TTC_SendACK(pkt.sequence_id, pkt.command_id, ERR_INVALID_PARAM);
                return;
            }
            
            TTC_SendACK(pkt.sequence_id, pkt.command_id, ACK_OK);
            vTaskDelay(pdMS_TO_TICKS(100));
            esp_restart();
            break;
        }

        default:
            TTC_SendACK(pkt.sequence_id, pkt.command_id, ERR_INVALID_CMD);
            break;
    }
}

bool TTC_CheckIncomingCommands(void) {
    // TODO: Quando o driver do LoRa receber um buffer de bytes:
    // TelecommandPacket_t pkt;
    // memcpy(&pkt, buffer_lora, sizeof(TelecommandPacket_t));
    // TTC_ProcessPacket(pkt);
    return false;
}

// --- Interface Padrão do Módulo TT&C ---

SubsystemStatus_t TTC_Init(void) {
    // TODO: Inicializar SPI e registradores do transceptor SX1276
    return SUBSYSTEM_OK;
}

void TTC_Task(void *pvParameters) {
    for (;;) {
        TTC_CheckIncomingCommands();
        vTaskDelay(pdMS_TO_TICKS(100)); // Polling a 10Hz
    }
}

SubsystemStatus_t TTC_GetTelemetry(TTC_Telemetry_t *out_telemetry) {
    if (out_telemetry == NULL) {
        return SUBSYSTEM_ERR_INVALID_PARAM;
    }
    *out_telemetry = g_ttc_telemetry;
    return SUBSYSTEM_OK;
}

SubsystemStatus_t TTC_HandleCommand(uint8_t command_id, const uint8_t *payload, uint16_t length) {
    // Tratamento de reconfigurações do próprio rádio (ex: alterar Frequência, Potência, etc)
    return SUBSYSTEM_OK;
}

bool TTC_HealthCheck(void) {
    // TODO: Ler registrador de versão do SX1276 via SPI para validar a presença do CI
    return true;
}
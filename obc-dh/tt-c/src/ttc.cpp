#include "ttc.h"
#include <RTClib.h>
#include "sd_logger.h"

// Referência global do RTC declarada no main.cpp
extern RTC_DS3231 rtc;

// Calculador de integridade CRC-16 (Modbus/CCITT)
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

bool TTC_Init() {
    // TODO: Inicializar hardware do SX1276 via VSPI
    return true;
}

bool TTC_SendTelemetry(const PacketTelemetry &packet) {
    // TODO: Enviar pacote via LoRa
    return true;
}

void TTC_SendACK(uint16_t sequence_id, uint8_t command_id, CmdStatus status) {
    Serial.printf("[TT&C] ACK | SEQ: %u | CMD: 0x%02X | Status: 0x%02X\n", 
                  sequence_id, command_id, status);
    // TODO: Transmitir pacote ACK/NACK via LoRa
}

// Processador central de pacotes (Dispatcher)
void TTC_ProcessPacket(const TelecommandPacket &pkt) {
    // 1. Validação de integridade CRC16
    uint16_t computed_crc = TTC_CalculateCRC16((const uint8_t*)&pkt, sizeof(TelecommandPacket) - sizeof(uint16_t));
    if (computed_crc != pkt.checksum) {
        TTC_SendACK(pkt.sequence_id, pkt.command_id, ERR_CRC);
        return;
    }

    // 2. Execução por Opcode
    switch (pkt.command_id) {
        case CMD_PING:
            TTC_SendACK(pkt.sequence_id, pkt.command_id, ACK_OK);
            break;

        case CMD_SET_TIME: {
            // Extrai o uint32_t dos primeiros 4 bytes do buffer de argumentos
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
            if (burn_time > 10) { // Trava de segurança térmica
                TTC_SendACK(pkt.sequence_id, pkt.command_id, ERR_INVALID_PARAM);
                return;
            }
            
            SD_Log("DEPLOY", "ANTENNA", LOG_EVENT, "Acionamento do Burn Wire");
            digitalWrite(33, HIGH); // PINO BURN
            delay(burn_time * 1000);
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

bool TTC_CheckIncomingCommands() {
    // TODO: Quando o driver do LoRa receber um buffer de bytes da antena:
    // TelecommandPacket pkt;
    // memcpy(&pkt, buffer_lora, sizeof(TelecommandPacket));
    // TTC_ProcessPacket(pkt);
    return false;
}
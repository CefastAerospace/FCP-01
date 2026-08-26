#include "ttc.h"
#include <RTClib.h>
#include "sd_logger.h"
#include "conops.h"

// Instância global da telemetria de TT&C
TTC_Telemetry_t g_ttc_telemetry = {0, 0, 0, 0};

// Tabela dinâmica/algoritmo de CRC16 CCITT (0x1021)
uint16_t TTC_CalculateCRC16(const uint8_t *data, size_t length) {
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

SubsystemStatus_t TTC_Init() {
    // Configuração de registradores do módulo de rádio via SPI (SX1276/RFM95)
    // Pino CS, RST, IRQ etc.
    pinMode(33, OUTPUT); // Pino do Burn Wire
    digitalWrite(33, LOW);

    g_ttc_telemetry.rx_packets_count = 0;
    g_ttc_telemetry.tx_packets_count = 0;
    g_ttc_telemetry.rx_errors_count = 0;
    g_ttc_telemetry.last_rssi = -120;

    return SUBSYSTEM_OK;
}

void TTC_SendACK(uint16_t sequence_id, uint8_t command_id, uint8_t status_code) {
    ACKPacket_t ack;
    ack.header = 0xAA55;
    ack.sequence_id = sequence_id;
    ack.command_id = command_id;
    ack.status_code = status_code;
    ack.checksum = TTC_CalculateCRC16((const uint8_t*)&ack, sizeof(ACKPacket_t) - sizeof(uint16_t));

    // Envio físico via rádio (SPI/LoRa)
    // LoRa.beginPacket(); LoRa.write((uint8_t*)&ack, sizeof(ACKPacket_t)); LoRa.endPacket();
    
    g_ttc_telemetry.tx_packets_count++;
}

void TTC_ProcessPacket(const TelecommandPacket_t &pkt) {
    // 1. Validação de Integridade via CRC16
    uint16_t computed_crc = TTC_CalculateCRC16((const uint8_t*)&pkt, sizeof(TelecommandPacket_t) - sizeof(uint16_t));
    if (computed_crc != pkt.checksum) {
        g_ttc_telemetry.rx_errors_count++;
        TTC_SendACK(pkt.sequence_id, pkt.command_id, ERR_CRC);
        return;
    }

    g_ttc_telemetry.rx_packets_count++;

    // 2. Processamento do Comando recebido via Ground Station
    switch (pkt.command_id) {
        
        // --- COMANDOS DE MUDANÇA DE ESTADO (CONOPS) ---
        
        case CMD_ENTER_SAFE: {
            // ALTERAÇÃO CONOPS: Trocou systemState = STATE_SAFE por chamada thread-safe
            if (System_SetState(STATE_SAFE)) {
                SD_Log("CONOPS", "CMD", LOG_EVENT, "SAFE MODE ativado via Ground Station");
                TTC_SendACK(pkt.sequence_id, pkt.command_id, ACK_OK);
            } else {
                TTC_SendACK(pkt.sequence_id, pkt.command_id, ERR_INVALID_CMD);
            }
            break;
        }

        case CMD_EXIT_SAFE: {
            // Requer chave de segurança nos argumentos para evitar saída acidental de SAFE
            uint16_t confirm_key = *(uint16_t*)&pkt.arguments[0];
            if (confirm_key != 0xA55A) {
                TTC_SendACK(pkt.sequence_id, pkt.command_id, ERR_INVALID_PARAM);
                return;
            }

            // ALTERAÇÃO CONOPS: Transição segura de volta para MISSION
            if (System_SetState(STATE_MISSION)) {
                SD_Log("CONOPS", "CMD", LOG_EVENT, "Retorno para MISSION MODE via Ground Station");
                TTC_SendACK(pkt.sequence_id, pkt.command_id, ACK_OK);
            } else {
                TTC_SendACK(pkt.sequence_id, pkt.command_id, ERR_INVALID_CMD);
            }
            break;
        }

        case CMD_SET_MODE: {
            SystemState_t targetState = (SystemState_t)pkt.arguments[0];
            if (System_SetState(targetState)) {
                SD_Log("CONOPS", "CMD", LOG_EVENT, "Estado alterado manualmente via GS");
                TTC_SendACK(pkt.sequence_id, pkt.command_id, ACK_OK);
            } else {
                TTC_SendACK(pkt.sequence_id, pkt.command_id, ERR_INVALID_PARAM);
            }
            break;
        }

        // --- COMANDOS OPERACIONAIS DA MISSÃO ---

        case CMD_DEPLOY_ANTENNA: {
            // ALTERAÇÃO CONOPS: Trocou if(systemState != STATE_MISSION) pela consulta segura
            if (System_GetState() != STATE_MISSION) {
                Serial.println("[TT&C] Bloqueado: Burn wire acionado fora do modo MISSION!");
                TTC_SendACK(pkt.sequence_id, pkt.command_id, ERR_INVALID_CMD);
                return;
            }

            uint8_t burn_time = pkt.arguments[0];
            if (burn_time > 10) { // Trava de segurança: máximo 10 segundos de acionamento
                TTC_SendACK(pkt.sequence_id, pkt.command_id, ERR_INVALID_PARAM);
                return;
            }
            
            SD_Log("DEPLOY", "ANTENNA", LOG_EVENT, "Acionamento do Burn Wire iniciado");
            digitalWrite(33, HIGH);
            vTaskDelay(pdMS_TO_TICKS(burn_time * 1000));
            digitalWrite(33, LOW);
            SD_Log("DEPLOY", "ANTENNA", LOG_EVENT, "Acionamento do Burn Wire concluido");
            
            TTC_SendACK(pkt.sequence_id, pkt.command_id, ACK_OK);
            break;
        }

        case CMD_PING: {
            TTC_SendACK(pkt.sequence_id, pkt.command_id, ACK_OK);
            break;
        }

        case CMD_REQUEST_TELEMETRY: {
            // Envia o pacote de telemetria compilado de volta para a GS
            TTC_SendTelemetryPacket();
            TTC_SendACK(pkt.sequence_id, pkt.command_id, ACK_OK);
            break;
        }

        default: {
            TTC_SendACK(pkt.sequence_id, pkt.command_id, ERR_UNKNOWN_CMD);
            break;
        }
    }
}

void TTC_SendTelemetryPacket() {
    // Monta pacote com os dados atuais do satélite e envia via rádio
    g_ttc_telemetry.tx_packets_count++;
}

void TaskTTC(void *parameter) {
    TelecommandPacket_t rx_packet;

    for (;;) {
        // Exemplo: se houver pacote recebido no FIFO do módulo LoRa
        /*
        if (LoRa_HasPacket()) {
            LoRa_ReadPacket((uint8_t*)&rx_packet, sizeof(TelecommandPacket_t));
            g_ttc_telemetry.last_rssi = LoRa_GetRSSI();
            TTC_ProcessPacket(rx_packet);
        }
        */

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

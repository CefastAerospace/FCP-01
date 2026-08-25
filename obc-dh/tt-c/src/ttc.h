#ifndef TTC_H
#define TTC_H

#include <Arduino.h>

// Status de resposta dos Telecomandos (ACK/NACK)
enum CmdStatus : uint8_t {
    ACK_OK               = 0x00,
    ERR_CRC              = 0x01,
    ERR_INVALID_CMD      = 0x02,
    ERR_INVALID_PARAM    = 0x03,
    ERR_STATE_REJECTED   = 0x04
};

// Identificadores de Telecomandos (Opcodes de Entrada)
enum CommandID : uint8_t {
    CMD_PING           = 0x01,
    CMD_SET_MODE       = 0x02,
    CMD_START_MISSION  = 0x03,
    CMD_STOP_MISSION   = 0x04,
    CMD_GET_STATUS     = 0x05,
    CMD_SET_TIME       = 0x10,
    CMD_ADCS_START     = 0x20,
    CMD_ADCS_STOP      = 0x21,
    CMD_DEPLOY_ANTENNA = 0x30,
    CMD_REQUEST_LOG    = 0x40,
    CMD_RESET_OBC      = 0xFF
};

// Estrutura de dados do pacote de telemetria para envio via LoRa (TM)
struct PacketTelemetry {
    uint32_t timestamp;
    float voltage;
    float current;
    float temp;
    uint8_t systemStatus;
};

// Estrutura genérica e compacta do Telecomando vindo da Ground Station (TC)
struct __attribute__((packed)) TelecommandPacket {
    uint16_t sequence_id;   // Contador sequencial anti-replay
    uint8_t  command_id;    // Opcode do comando (CommandID)
    uint8_t  flags;         // Bit 0: Requisição de ACK
    uint32_t timestamp;     // Timestamp da emissão ou execução
    uint8_t  arguments[8];  // Buffer fixo para parâmetros variáveis
    uint16_t checksum;      // CRC16 de integridade
};

// Protótipos das Funções
bool TTC_Init();
bool TTC_SendTelemetry(const PacketTelemetry &packet);

// Leitura e despacho de comandos recebidos via rádio
bool TTC_CheckIncomingCommands();
void TTC_ProcessPacket(const TelecommandPacket &pkt);
void TTC_SendACK(uint16_t sequence_id, uint8_t command_id, CmdStatus status);

// Algoritmo de validação de integridade
uint16_t TTC_CalculateCRC16(const uint8_t *data, size_t len);

#endif // TTC_H
#ifndef TTC_H
#define TTC_H

#include <Arduino.h>

// Identificadores de Telecomandos (Opcodes de Entrada)
enum CommandID : uint8_t {
    CMD_PING        = 0x01,
    CMD_SET_TIME    = 0x10, // Sincronização do RTC
    CMD_RESET_OBC   = 0xFF
};

// Estrutura de dados do pacote de telemetria para envio via LoRa (TM)
struct PacketTelemetry {
    uint32_t timestamp;
    float voltage;
    float current;
    float temp;
    uint8_t systemStatus;
};

// Estrutura do Telecomando TC_SET_TIME vindo da Ground Station (TC)
struct __attribute__((packed)) TC_SetTime {
    uint8_t command_id;     // Deve ser CMD_SET_TIME (0x10)
    uint32_t unix_timestamp;// Timestamp UTC enviado pela GS
    uint16_t checksum;      // CRC16 para validação
};

// Protótipos das Funções
bool TTC_Init();
bool TTC_SendTelemetry(const PacketTelemetry &packet);

// Processa pacotes recebidos e retorna verdadeiro se um comando válido foi executado
bool TTC_CheckIncomingCommands();

// Processa especificamente o pacote de ajuste de tempo
bool TTC_HandleSetTimeCommand(const TC_SetTime &cmd);

#endif // TTC_H
#ifndef TTC_H
#define TTC_H

#include <Arduino.h>

// Estrutura de dados do pacote de telemetria para envio via LoRa
struct PacketTelemetry {
    uint32_t timestamp;
    float voltage;
    float current;
    float temp;
    uint8_t systemStatus;
};

// Protótipos das Funções
bool TTC_Init();
bool TTC_SendTelemetry(const PacketTelemetry &packet);
bool TTC_CheckIncomingCommands();

#endif // TTC_H
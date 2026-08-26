#ifndef EPS_H
#define EPS_H

#include <Arduino.h>
#include "subsystem_interface.h"

// Estrutura de Telemetria do EPS (packed para garantir alinhamento no envio por rádio)
typedef struct __attribute__((packed)) {
    float bus_voltage; // Tensão do barramento (V)
    float crnt_mA;     // Corrente da bateria (mA)
    float pwr_mW;      // Potência consumida (mW)
    float temp_C;      // Temperatura da Placa OBC (°C)
} EPS_Telemetry_t;

// --- Classe do Módulo EPS derivada da ISubsystem ---
class EPS_Module : public ISubsystem {
private:
    EPS_Telemetry_t _telemetry;

public:
    EPS_Module();

    // --- Implementação do Contrato ISubsystem ---
    SubsystemStatus_t Init() override;
    void TaskUpdate() override;
    SubsystemStatus_t GetTelemetry(uint8_t *buffer, size_t max_len, size_t *out_len) override;
    SubsystemStatus_t HandleCommand(const SubsystemCommand_t &cmd) override;
    bool HealthCheck() override;
    const char* GetName() override { return "EPS"; }

    // --- Método Auxiliar de Leitura de Dados ---
    EPS_Telemetry_t ReadData();
};

extern EPS_Module EPS;

#endif // EPS_H

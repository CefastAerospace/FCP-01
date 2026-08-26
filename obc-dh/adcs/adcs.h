#ifndef ADCS_H
#define ADCS_H

#include <Arduino.h>
#include "subsystem_interface.h"

// Modos de operação do ADCS
typedef enum : uint8_t {
    ADCS_MODE_IDLE = 0,
    ADCS_MODE_ACTIVE,
    ADCS_MODE_EMERGENCY_STOP
} ADCS_Mode_t;

// Estrutura de dados do sensor IMU (Aceleração e Velocidade Angular)
typedef struct __attribute__((packed)) {
    float ax, ay, az; // Aceleração (m/s²)
    float gx, gy, gz; // Velocidade Angular (rad/s ou °/s)
} IMUData_t;

// Estrutura completa de telemetria do ADCS
typedef struct __attribute__((packed)) {
    IMUData_t imu;          // Dados dos sensores de orientação
    float current_rpm;      // Velocidade atual da roda de reação (RPM)
    float target_rpm;       // Velocidade alvo da roda de reação (RPM)
    ADCS_Mode_t mode;       // Modo operacional atual
} ADCS_Telemetry_t;

// Classe do Módulo ADCS estendendo o contrato ISubsystem
class ADCS_Module : public ISubsystem {
private:
    ADCS_Telemetry_t _telemetry;

public:
    ADCS_Module();

    // Métodos da Interface Padrão (ISubsystem)
    SubsystemStatus_t Init() override;
    void TaskUpdate() override;
    SubsystemStatus_t GetTelemetry(uint8_t *buffer, size_t max_len, size_t *out_len) override;
    SubsystemStatus_t HandleCommand(const SubsystemCommand_t &cmd) override;
    bool HealthCheck() override;
    const char* GetName() override { return "ADCS"; }

    // Funções Específicas do Hardware (SimpleFOC / MPU6050)
    IMUData_t ReadIMU();
    void SetMotorSpeed(float targetRPM);
    void EmergencyStop();
};

extern ADCS_Module ADCS;

#endif // ADCS_H

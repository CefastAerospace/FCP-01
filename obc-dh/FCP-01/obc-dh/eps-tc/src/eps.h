#ifndef EPS_H
#define EPS_H

#include <Arduino.h>

// Armazenar telemetria do EPS
struct EPS_Telemetry {
    float bus_voltage; // Tensão do barramento
    float crnt_mA; // Corrente da bateria
    float pwr_mW; // Potência consumida
    float temp_C; // Temperatura da Placa OBC
};

bool EPSinit();
EPS_Telemetry EPS_ReadData();
bool EPS_CheckSafety(const EPS_Telemetry& data);

#endif // EPS_H
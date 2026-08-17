#ifndef ADCS_H
#define ADCS_H

#include <Arduino.h>

// Estrutura com os dados do sensor de orientação
struct IMUData {
    float ax, ay, az; // Aceleração
    float gx, gy, gz; // Velocidade Angular
};

// Protótipos das Funções
bool ADCS_Init();
IMUData ADCS_ReadIMU();
void ADCS_SetMotorSpeed(float targetRPM);
void ADCS_EmergencyStop();

#endif // ADCS_H
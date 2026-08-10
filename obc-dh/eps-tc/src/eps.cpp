#include "eps.h"

// Inicializa os sensores do EPS (INA219, LM75A, etc.)
bool EPSinit() {
    // Aqui no futuro vai o Wire.begin() e as verificações do INA219
    return true; 
}

// Leitura dos dados do EPS
EPS_Telemetry EPS_ReadData() {
    EPS_Telemetry data;

    // Dados simulados (Mocks) para testes de software sem sensores
    data.bus_voltage = 3.3f; // 3.3V
    data.crnt_mA = 150.0f;   // 150mA
    data.pwr_mW = 495.0f;    // 495mW
    data.temp_C = 24.5f;     // 24.5°C

    return data;
}


bool EPS_CheckSafety(const EPS_Telemetry& data) {
    if (data.bus_voltage < 3.0f || data.temp_C > 60.0f) {
        return false;
    }
    return true;
}
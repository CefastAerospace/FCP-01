#include "eps.h"

// Instância global do módulo EPS
EPS_Module EPS;

EPS_Module::EPS_Module() {
    _telemetry.bus_voltage = 0.0f;
    _telemetry.crnt_mA = 0.0f;
    _telemetry.pwr_mW = 0.0f;
    _telemetry.temp_C = 0.0f;
}

SubsystemStatus_t EPS_Module::Init() {
    // Inicialização dos sensores do EPS (ex: INA219, LM75A via Wire.begin())
    _telemetry = ReadData();
    return SUBSYS_OK;
}

void EPS_Module::TaskUpdate() {
    // Atualiza periodicamente as leituras do sistema de energia
    _telemetry = ReadData();
}

EPS_Telemetry_t EPS_Module::ReadData() {
    EPS_Telemetry_t data;

    // Dados simulados (Mocks) para testes de software sem sensores físicos conectados
    data.bus_voltage = 3.3f; // 3.3V
    data.crnt_mA = 150.0f;   // 150mA
    data.pwr_mW = 495.0f;    // 495mW
    data.temp_C = 24.5f;     // 24.5°C

    return data;
}

SubsystemStatus_t EPS_Module::GetTelemetry(uint8_t *buffer, size_t max_len, size_t *out_len) {
    if (!buffer || !out_len) return SUBSYS_ERR_PARAM_INVALID;
    if (max_len < sizeof(EPS_Telemetry_t)) return SUBSYS_ERR_PARAM_INVALID;

    memcpy(buffer, &_telemetry, sizeof(EPS_Telemetry_t));
    *out_len = sizeof(EPS_Telemetry_t);
    return SUBSYS_OK;
}

SubsystemStatus_t EPS_Module::HandleCommand(const SubsystemCommand_t &cmd) {
    switch (cmd.command_id) {
        case 0x10: // Comando de reset ou reconfiguração do barramento
            return SUBSYS_OK;

        default:
            return SUBSYS_ERR_CMD_UNKNOWN;
    }
}

bool EPS_Module::HealthCheck() {
    // Validação de segurança baseada na função EPS_CheckSafety original
    if (_telemetry.bus_voltage < 3.0f || _telemetry.temp_C > 60.0f) {
        return false;
    }
    return true;
}

#include "adcs.h"

// Instância global do módulo ADCS
ADCS_Module ADCS;

ADCS_Module::ADCS_Module() {
    _telemetry.imu.ax = 0.0f;
    _telemetry.imu.ay = 0.0f;
    _telemetry.imu.az = 0.0f;
    _telemetry.imu.gx = 0.0f;
    _telemetry.imu.gy = 0.0f;
    _telemetry.imu.gz = 0.0f;
    _telemetry.current_rpm = 0.0f;
    _telemetry.target_rpm = 0.0f;
    _telemetry.mode = ADCS_MODE_IDLE;
}

SubsystemStatus_t ADCS_Module::Init() {
    // 1. Inicialização do Barramento I2C e MPU6050
    // Wire.begin();
    // mpu.initialize();

    // 2. Inicialização do Driver/Motor SimpleFOC
    // driver.init();
    // motor.linkDriver(&driver);
    // motor.init();
    // motor.initFOC();

    _telemetry.mode = ADCS_MODE_IDLE;
    _telemetry.current_rpm = 0.0f;
    _telemetry.target_rpm = 0.0f;

    return SUBSYS_OK;
}

void ADCS_Module::TaskUpdate() {
    // 1. Atualiza leitura da IMU
    _telemetry.imu = ReadIMU();

    // 2. Executa loop do SimpleFOC (em produção, chame motor.loopFOC())
    // motor.loopFOC();

    // 3. Controle de malha fechada / Atuação do motor conforme o modo atual
    switch (_telemetry.mode) {
        case ADCS_MODE_ACTIVE:
            // Ajusta a velocidade atual em direção à target_rpm
            // motor.move(_telemetry.target_rpm);
            _telemetry.current_rpm = _telemetry.target_rpm; // Simulação de ramp up
            break;

        case ADCS_MODE_EMERGENCY_STOP:
        case ADCS_MODE_IDLE:
        default:
            _telemetry.target_rpm = 0.0f;
            _telemetry.current_rpm = 0.0f;
            // motor.move(0);
            break;
    }
}

SubsystemStatus_t ADCS_Module::GetTelemetry(uint8_t *buffer, size_t max_len, size_t *out_len) {
    if (buffer == NULL || out_len == NULL) return SUBSYS_ERR_PARAM_INVALID;
    if (max_len < sizeof(ADCS_Telemetry_t)) return SUBSYS_ERR_PARAM_INVALID;

    memcpy(buffer, &_telemetry, sizeof(ADCS_Telemetry_t));
    *out_len = sizeof(ADCS_Telemetry_t);
    return SUBSYS_OK;
}

SubsystemStatus_t ADCS_Module::HandleCommand(const SubsystemCommand_t &cmd) {
    switch (cmd.command_id) {
        case 0x20: // CMD_ADCS_START
            _telemetry.mode = ADCS_MODE_ACTIVE;
            return SUBSYS_OK;

        case 0x21: // CMD_ADCS_STOP
            _telemetry.mode = ADCS_MODE_IDLE;
            SetMotorSpeed(0.0f);
            return SUBSYS_OK;

        case 0x22: // CMD_ADCS_SET_RPM (exemplo de parâmetro vindo no payload)
            if (cmd.payload_len >= sizeof(float)) {
                float requested_rpm;
                memcpy(&requested_rpm, cmd.payload, sizeof(float));
                SetMotorSpeed(requested_rpm);
                return SUBSYS_OK;
            }
            return SUBSYS_ERR_PARAM_INVALID;

        case 0xFF: // Emergency Stop
            EmergencyStop();
            return SUBSYS_OK;

        default:
            return SUBSYS_ERR_CMD_UNKNOWN;
    }
}

bool ADCS_Module::HealthCheck() {
    // 1. Verifica se a velocidade da roda não excedeu limites físicos (ex: 5000 RPM)
    if (abs(_telemetry.current_rpm) > 5000.0f) {
        EmergencyStop();
        return false;
    }

    // 2. Verifica se a IMU está respondendo (ex: se os valores não estão travados em NaN/0 constante)
    if (_telemetry.mode == ADCS_MODE_EMERGENCY_STOP) {
        return false;
    }

    return true;
}

IMUData_t ADCS_Module::ReadIMU() {
    IMUData_t data;

    // Leitura real via biblioteca do MPU6050
    // int16_t ax, ay, az, gx, gy, gz;
    // mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);

    // Valores padrão / dummy para compilação inicial
    data.ax = 0.0f;
    data.ay = 0.0f;
    data.az = 9.81f;
    data.gx = 0.0f;
    data.gy = 0.0f;
    data.gz = 0.0f;

    return data;
}

void ADCS_Module::SetMotorSpeed(float targetRPM) {
    if (_telemetry.mode == ADCS_MODE_EMERGENCY_STOP) return;

    _telemetry.target_rpm = targetRPM;
    if (targetRPM != 0.0f) {
        _telemetry.mode = ADCS_MODE_ACTIVE;
    }
}

void ADCS_Module::EmergencyStop() {
    _telemetry.mode = ADCS_MODE_EMERGENCY_STOP;
    _telemetry.target_rpm = 0.0f;
    _telemetry.current_rpm = 0.0f;
    
    // Desliga driver físico do SimpleFOC
    // motor.disable();
}

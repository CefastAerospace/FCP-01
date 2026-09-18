#include "commands.h"
#include "config.h"
#include "sensors.h"
#include "motor_control.h"
#include "missions.h"

// ============================================================================
//  Implementação do Módulo de Comandos
//  Camada de abstração entre a interface (WiFi/LoRa) e a lógica do ADCS
// ============================================================================

// ---- Estado interno para spin test ----
static bool spinTestActive = false;
static unsigned long spinTestEnd = 0;

// ---- Inicialização ----
void cmd_init() {
    mission_init();
    Serial.println("[CMD] Camada de comandos inicializada.");
}

// ---- Atualização (chamada no loop) ----
void cmd_update() {
    // Gerencia o spin test temporizado
    if (spinTestActive && millis() >= spinTestEnd) {
        spinTestActive = false;
        motor_stop();
        Serial.println("[CMD] Spin test concluido.");
    }
}

// ---- Controle do Motor ----
void cmd_setVelocity(float rad_s) {
    mission_startManualTest();
    motor_setVelocity(rad_s);
}

void cmd_setAngle(float rad) {
    mission_startManualTest();
    motor_setAngle(rad);
}

void cmd_incrementVelocity(float delta_rad_s) {
    if (mission_getState() != MISSION_MANUAL_TEST) {
        mission_startManualTest();
    }
    float newVel = motor_getTarget() + delta_rad_s;
    motor_setVelocity(newVel);
}

void cmd_stopMotor() {
    spinTestActive = false;
    motor_stop();
    Serial.println("[CMD] Motor parado.");
}

void cmd_emergencyStop() {
    spinTestActive = false;
    mission_stop();
    motor_emergencyStop();
    Serial.println("[CMD] !!! EMERGENCIA — Tudo parado !!!");
}

void cmd_enableMotor() {
    motor_enable();
    Serial.println("[CMD] Motor habilitado.");
}

void cmd_spinTest(float velocity, unsigned long duration_ms) {
    mission_startManualTest();
    motor_setVelocity(velocity);
    spinTestActive = true;
    spinTestEnd = millis() + duration_ms;
    Serial.printf("[CMD] Spin test: %.1f rad/s por %lu ms\n", velocity, duration_ms);
}

// ---- Missões ----
void cmd_startDetumble() {
    mission_startDetumble();
}

void cmd_startPointing() {
    mission_startPointing();
}

void cmd_stopMission() {
    spinTestActive = false;
    mission_stop();
}

// ---- Telemetria JSON ----
String cmd_getStatusJSON() {
    const SensorData& s = sensors_getData();
    String json = "{";

    // Motor
    json += "\"motor\":{";
    json += "\"velocity\":" + String(motor_getVelocity(), 2) + ",";
    json += "\"angle\":" + String(motor_getAngle(), 2) + ",";
    json += "\"target\":" + String(motor_getTarget(), 2) + ",";
    json += "\"mode\":\"" + String(motor_getModeString()) + "\",";
    json += "\"enabled\":" + String(motor_isEnabled() ? "true" : "false");
    json += "},";

    // IMU
    json += "\"imu\":{";
    json += "\"accel_x\":" + String(s.accel_x, 3) + ",";
    json += "\"accel_y\":" + String(s.accel_y, 3) + ",";
    json += "\"accel_z\":" + String(s.accel_z, 3) + ",";
    json += "\"gyro_x\":" + String(s.gyro_x, 2) + ",";
    json += "\"gyro_y\":" + String(s.gyro_y, 2) + ",";
    json += "\"gyro_z\":" + String(s.gyro_z, 2) + ",";
    json += "\"angle_x\":" + String(s.angle_x, 2) + ",";
    json += "\"angle_y\":" + String(s.angle_y, 2) + ",";
    json += "\"angle_z\":" + String(s.angle_z, 2) + ",";
    json += "\"temp\":" + String(s.imu_temp, 1) + ",";
    json += "\"ok\":" + String(s.mpu_ok ? "true" : "false");
    json += "},";

    // Sensores de Luz
    json += "\"light\":{";
    json += "\"lux1\":" + String(s.lux_sensor1, 1) + ",";
    json += "\"lux2\":" + String(s.lux_sensor2, 1) + ",";
    json += "\"diff\":" + String(s.lux_diff, 1) + ",";
    json += "\"ok1\":" + String(s.bh1750_1_ok ? "true" : "false") + ",";
    json += "\"ok2\":" + String(s.bh1750_2_ok ? "true" : "false");
    json += "},";

    // Missão
    json += "\"mission\":{";
    json += "\"state\":\"" + String(mission_getStateString()) + "\",";
    json += "\"elapsed_ms\":" + String(mission_getElapsedTime());
    json += "}";

    json += "}";
    return json;
}


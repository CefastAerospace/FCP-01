#ifndef SENSORS_H
#define SENSORS_H

#include <Arduino.h>

// ============================================================================
//  Módulo de Sensores — MPU9250 + 2x BH1750
//  Leitura periódica e não-bloqueante dos sensores de atitude e luminosidade
// ============================================================================

// Estrutura com todos os dados dos sensores
struct SensorData {
    // MPU9250 — Acelerômetro (g)
    float accel_x;
    float accel_y;
    float accel_z;
    float resultant_g;

    // MPU9250 — Giroscópio (°/s)
    float gyro_x;
    float gyro_y;
    float gyro_z;

    // MPU9250 — Ângulos estimados (°)
    float angle_x;
    float angle_y;
    float angle_z;

    // MPU9250 — Temperatura interna (°C)
    float imu_temp;

    // BH1750 — Sensores de luz (lux)
    float lux_sensor1;   // ADDR = GND (0x23)
    float lux_sensor2;   // ADDR = VCC (0x5C)
    float lux_diff;      // sensor1 - sensor2 (para sun pointing)

    // Status
    bool mpu_ok;
    bool bh1750_1_ok;
    bool bh1750_2_ok;
    unsigned long last_update;
};

// Inicializa todos os sensores (chamar após Wire.begin())
void sensors_init();

// Atualiza leituras (non-blocking, usa millis() internamente)
void sensors_update();

// Retorna referência aos dados atuais
const SensorData& sensors_getData();

// Imprime dados no Serial (para debug)
void sensors_printSerial();

#endif // SENSORS_H

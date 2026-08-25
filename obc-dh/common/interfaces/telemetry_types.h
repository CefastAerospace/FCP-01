#ifndef TELEMETRY_TYPES_H
#define TELEMETRY_TYPES_H

#include <stdint.h>

// Estrutura de Atitude (ADCS / IMU)
typedef struct __attribute__((packed)) {
    float gyro_x, gyro_y, gyro_z;      // Velocidade angular (deg/s)
    float accel_x, accel_y, accel_z;   // Aceleração linear (g)
    float mag_x, mag_y, mag_z;         // Campo magnético (uT)
} ADCS_Data_t;

// Estrutura do Sistema Elétrico (EPS)
typedef struct __attribute__((packed)) {
    float vbat;                        // Tensão da bateria (V)
    float ibat;                        // Corrente total de consumo (mA)
    float v_panel;                     // Tensão dos painéis solares (V)
    float i_panel;                     // Corrente de carga solar (mA)
} EPS_Data_t;

// Pacote Unificado de Telemetria do Sistema (TM) - Atende HLR-COMM-02
typedef struct __attribute__((packed)) {
    uint32_t timestamp;                // Timestamp Unix (RTC)
    uint8_t  system_state;             // Estado do Sistema (Ex: BOOT, NOMINAL, SAFE, MISSION)
    
    EPS_Data_t  eps;                   // Parâmetros elétricos
    ADCS_Data_t adcs;                  // Atitude e dados de movimento
    
    float temp_obc;                    // Temperatura interna da OBC (°C)
    float temp_bat;                    // Temperatura da bateria (°C)
    
    uint16_t mission_data_counter;     // Dados de missão / leituras da Carga Útil
    uint16_t error_code;               // Código do último erro/diagnóstico gravado
    uint32_t system_flags;             // Status bits (Ex: bit 0: SD montado, bit 1: Antena implantada)
} SystemTelemetry_t;

#endif // TELEMETRY_TYPES_H
// ============================================================================
//  ADCS — Sistema de Controle de Atitude (Attitude Determination & Control)
//  CubeSat — Firmware Modular
//
//  Hardware:
//    - ESP32
//    - SimpleFOC Mini (DRV8313) + Motor BLDC (roda de reação)
//    - AS5600 (encoder magnético)
//    - MPU9250 (IMU: acelerômetro + giroscópio)
//    - 2x BH1750 (sensores de luminosidade)
//
//  Arquitetura:
//    config.h          → Configurações de pinos, parâmetros, WiFi
//    sensors.h/.cpp    → Leitura do MPU9250 + BH1750
//    motor_control     → Controle FOC da roda de reação
//    missions          → Algoritmos de Detumble e Sun Pointing
//    commands          → Camada de abstração (interface-agnostic)
//    web_interface     → Dashboard HTML via WiFi (CAMADA TROCÁVEL)
//
//  Para trocar WiFi por LoRa:
//    1. Crie lora_interface.h/.cpp com lora_init() e lora_loop()
//    2. Troque o #include e as chamadas abaixo
//    3. Nenhum outro arquivo precisa mudar!
// ============================================================================

#include <Arduino.h>
#include <Wire.h>

#include "include/config.h"
#include "include/sensors.h"
#include "include/motor_control.h"
#include "include/missions.h"
#include "include/commands.h"
#include "include/web_interface.h"    // ← TROCAR POR "lora_interface.h" no futuro

// Timer para telemetria Serial
static unsigned long lastSerialPrint = 0;

void setup() {
    Serial.begin(115200);
    while (!Serial && millis() < 3000);  // Aguarda Serial (max 3s)

    Serial.println();
    Serial.println("==========================================");
    Serial.println("  ADCS — CubeSat — Inicializando...");
    Serial.println("==========================================");

    // Inicializa barramento I2C
    Wire.begin(PIN_SDA, PIN_SCL);
    Wire.setClock(I2C_CLOCK);
    Serial.println("[MAIN] I2C inicializado.");

    // Inicializa módulos (ordem importa!)
    sensors_init();       // 1. Sensores primeiro
    motor_init();         // 2. Motor (usa I2C para AS5600)
    cmd_init();           // 3. Camada de comandos + missões
    web_init();           // 4. Interface WiFi  ← TROCAR POR lora_init()

    Serial.println();
    Serial.println("==========================================");
    Serial.println("  ADCS PRONTO!");
    Serial.println("==========================================");
    Serial.println();
}

void loop() {
    // ---- CRÍTICO: Loop FOC deve rodar na máxima frequência ----
    // NÃO coloque delay() nem operações bloqueantes aqui!
    motor_loopFOC();

    // ---- Atualizações periódicas (non-blocking via millis) ----
    sensors_update();       // Lê sensores (a cada SENSOR_UPDATE_INTERVAL_MS)
    mission_update();       // Atualiza missão ativa (a cada MISSION_UPDATE_INTERVAL_MS)
    cmd_update();           // Atualiza temporizadores de comandos

    // ---- Interface ----
    web_loop();             // Processa requisições HTTP  ← TROCAR POR lora_loop()

    // ---- Telemetria Serial (debug) ----
    unsigned long now = millis();
    if (now - lastSerialPrint >= TELEMETRY_PRINT_INTERVAL_MS) {
        lastSerialPrint = now;
        sensors_printSerial();
        Serial.printf("[MOTOR] Vel: %.2f rad/s | Target: %.2f | Modo: %s | %s\n",
                      motor_getVelocity(), motor_getTarget(),
                      motor_getModeString(),
                      motor_isEnabled() ? "ATIVO" : "DESABILITADO");
        Serial.printf("[MISSION] %s | Tempo: %lu s\n",
                      mission_getStateString(),
                      mission_getElapsedTime() / 1000);
        Serial.println("------------------------------------------");
    }
}


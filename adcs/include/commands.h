#ifndef COMMANDS_H
#define COMMANDS_H

#include <Arduino.h>

// ============================================================================
//  Módulo de Comandos — Camada de Abstração
//  Todas as ações do ADCS passam por aqui.
//  A interface (WiFi, LoRa, Serial) chama estas funções.
//  Ao trocar a interface, estas funções NÃO mudam.
// ============================================================================

// Inicializa o módulo de comandos
void cmd_init();

// Atualiza temporizadores internos (spin test, etc.)
void cmd_update();

// ---- Controle do Motor (testes/debug) ----
void cmd_setVelocity(float rad_s);
void cmd_setAngle(float rad);
void cmd_incrementVelocity(float delta_rad_s);   // +/- rad/s
void cmd_stopMotor();
void cmd_emergencyStop();
void cmd_enableMotor();

// Teste de rotação: gira por um tempo e para
void cmd_spinTest(float velocity, unsigned long duration_ms);

// ---- Missões ----
void cmd_startDetumble();
void cmd_startPointing();
void cmd_stopMission();

// ---- Telemetria ----
// Retorna string JSON com todos os dados do ADCS
String cmd_getStatusJSON();

#endif // COMMANDS_H

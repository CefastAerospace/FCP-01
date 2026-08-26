#ifndef CONOPS_H
#define CONOPS_H

#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

// Estados do Satélite (Máquina de Estados ConOps)
typedef enum {
    STATE_STARTUP = 0,
    STATE_BOOT_CHECK,
    STATE_MISSION,
    STATE_SAFE
} SystemState_t;

// --- Bitmasks para o EventGroup dos Subsistemas ---
#define SYS_BIT_EPS_READY       (1 << 0)
#define SYS_BIT_ADCS_READY      (1 << 1)
#define SYS_BIT_TTC_READY       (1 << 2)
#define SYS_BIT_SD_READY        (1 << 3)
#define SYS_BIT_RTC_READY       (1 << 4)
#define SYS_BIT_INIT_ERROR      (1 << 5)

// Máscara combinada: Todos os sistemas essenciais para entrar em missão
#define SYS_BITS_ALL_CRITICAL   (SYS_BIT_EPS_READY | SYS_BIT_ADCS_READY | SYS_BIT_TTC_READY)

// Handle global do EventGroup
extern EventGroupHandle_t xSystemEventGroup;

// --- Protótipos de Funções do ConOps ---
void System_Init();
SystemState_t System_GetState();
void System_SetState(SystemState_t new_state);
void System_SetSubsystemReady(EventBits_t bit);
bool System_WaitAllCriticalReady(TickType_t xTicksToWait);

#endif // CONOPS_H

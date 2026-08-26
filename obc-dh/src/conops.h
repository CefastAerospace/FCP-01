#ifndef CONOPS_H
#define CONOPS_H
#include <Arduino.h>

// Mapeamento oficial dos estados do ConOps
typedef enum {
    STATE_BOOT,
    STATE_INIT,
    STATE_SAFE,
    STATE_READY,
    STATE_MISSION
} SystemState_t;

// Opcodes para controle manual via Ground Station (TT&C)
#define CMD_ENTER_SAFE  0x10
#define CMD_EXIT_SAFE   0x11
#define CMD_SET_MODE    0x12

// Interface pública de gerenciamento de estado
bool System_InitConOps();
bool System_SetState(SystemState_t newState);
SystemState_t System_GetState();
const char* System_GetStateName(SystemState_t st);

#endif // CONOPS_H

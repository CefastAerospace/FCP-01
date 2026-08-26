#include "conops.h"

// Instância global do Event Group do FreeRTOS
EventGroupHandle_t xSystemEventGroup = NULL;

// Estado atual do sistema
static SystemState_t current_state = STATE_STARTUP;

void System_Init() {
    // Instancia o grupo de eventos do FreeRTOS caso ainda não exista
    if (xSystemEventGroup == NULL) {
        xSystemEventGroup = xEventGroupCreate();
    }
    current_state = STATE_STARTUP;
}

SystemState_t System_GetState() {
    return current_state;
}

void System_SetState(SystemState_t new_state) {
    current_state = new_state;
}

void System_SetSubsystemReady(EventBits_t bit) {
    if (xSystemEventGroup != NULL) {
        xEventGroupSetBits(xSystemEventGroup, bit);
    }
}

bool System_WaitAllCriticalReady(TickType_t xTicksToWait) {
    if (xSystemEventGroup == NULL) return false;

    // Aguarda até que TODOS os bits críticos (EPS, ADCS, TTC) estejam em nível 1
    EventBits_t uxBits = xEventGroupWaitBits(
        xSystemEventGroup,
        SYS_BITS_ALL_CRITICAL,
        pdFALSE,               // Não limpa os bits ao sair
        pdTRUE,                // Aguarda TODOS os bits especificados
        xTicksToWait
    );

    return (uxBits & SYS_BITS_ALL_CRITICAL) == SYS_BITS_ALL_CRITICAL;
}

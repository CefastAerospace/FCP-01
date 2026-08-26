#include "conops.h"
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#define PIN_EN   32
#define PIN_BURN 33

static SystemState_t currentState = STATE_BOOT;
static SemaphoreHandle_t stateMutex = NULL;

bool System_InitConOps() {
    if (stateMutex == NULL) {
        stateMutex = xSemaphoreCreateMutex();
    }
    if (stateMutex != NULL) {
        currentState = STATE_INIT;
        return true;
    }
    return false;
}

bool System_SetState(SystemState_t newState) {
    if (stateMutex == NULL) return false;

    if (xSemaphoreTake(stateMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        bool transitionAllowed = false;

        // Matriz de regras do ConOps
        switch (currentState) {
            case STATE_BOOT:
                if (newState == STATE_INIT) transitionAllowed = true;
                break;
            case STATE_INIT:
                if (newState == STATE_READY || newState == STATE_SAFE) transitionAllowed = true;
                break;
            case STATE_READY:
                if (newState == STATE_MISSION || newState == STATE_SAFE) transitionAllowed = true;
                break;
            case STATE_MISSION:
                if (newState == STATE_SAFE || newState == STATE_READY) transitionAllowed = true;
                break;
            case STATE_SAFE:
                // Liberação manual pela GS ou recuperação por re-init
                if (newState == STATE_INIT || newState == STATE_READY || newState == STATE_MISSION) {
                    transitionAllowed = true;
                }
                break;
        }

        if (transitionAllowed) {
            currentState = newState;
            
            // Trava de hardware imediata ao entrar em SAFE
            if (currentState == STATE_SAFE) {
                digitalWrite(PIN_BURN, LOW);
                digitalWrite(PIN_EN, LOW);
            }

            xSemaphoreGive(stateMutex);
            return true;
        }
        xSemaphoreGive(stateMutex);
    }
    return false;
}

SystemState_t System_GetState() {
    SystemState_t state = STATE_SAFE;
    if (stateMutex != NULL && xSemaphoreTake(stateMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
        state = currentState;
        xSemaphoreGive(stateMutex);
    }
    return state;
}

const char* System_GetStateName(SystemState_t st) {
    switch (st) {
        case STATE_BOOT:    return "BOOT";
        case STATE_INIT:    return "INIT";
        case STATE_SAFE:    return "SAFE";
        case STATE_READY:   return "READY";
        case STATE_MISSION: return "MISSION";
        default:            return "UNKNOWN";
    }
}

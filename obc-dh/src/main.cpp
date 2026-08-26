#include <Arduino.h>
#include "subsystem_interface.h"
#include "conops.h"
#include "ttc.h"
#include "eps.h"
#include "adcs.h"

// Handles das Tarefas do FreeRTOS
TaskHandle_t xTaskTTC_Handle    = NULL;
TaskHandle_t xTaskEPS_Handle    = NULL;
TaskHandle_t xTaskADCS_Handle   = NULL;
TaskHandle_t xTaskConOps_Handle = NULL;

// ==========================================
// TAREFAS FREERTOS (TASKS)
// ==========================================

/**
 * @brief Tarefa do TT&C (Telecomunicações) - Core 1
 * Trata o envio/recebimento de pacotes via rádio.
 */
void TaskTTC(void *pvParameters) {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(100); // 10 Hz

    for (;;) {
        TTC.TaskUpdate();
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}

/**
 * @brief Tarefa do ADCS (Controle de Atitude) - Core 1
 * Trata a leitura dos dados de IMU e o controle do motor SimpleFOC.
 */
void TaskADCS(void *pvParameters) {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(50); // 20 Hz

    for (;;) {
        ADCS.TaskUpdate();
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}

/**
 * @brief Tarefa do EPS (Energia) - Core 0
 * Trata a leitura dos sensores de tensão, corrente e temperatura.
 */
void TaskEPS(void *pvParameters) {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(500); // 2 Hz

    for (;;) {
        EPS.TaskUpdate();

        // Checagem contínua de saúde do EPS
        if (!EPS.HealthCheck()) {
            Serial.println("[EPS] ALERTA: Tensão baixa ou superaquecimento detectado!");
        }

        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}

/**
 * @brief Tarefa do ConOps (Supervisão de Estados) - Core 0
 * Trata as transições de estado de voo e monitoramento geral de segurança.
 */
void TaskConOps(void *pvParameters) {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(1000); // 1 Hz

    for (;;) {
        // Se a bateria cair em nível crítico durante a missão, força STATE_SAFE
        if (!EPS.HealthCheck() && System_GetState() == STATE_MISSION) {
            System_SetState(STATE_SAFE);
            Serial.println("[ConOps] Forçando STATE_SAFE devido a falha no EPS!");
        }

        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}

// ==========================================
// SETUP & LOOP
// ==========================================

void setup() {
    Serial.begin(115200);
    while (!Serial && millis() < 2000);

    Serial.println("\n=== [OBC-DH] Inicializando CubeSat ===");

    // 1. Inicializa o gerenciador do ConOps e o EventGroup
    System_Init();

    // 2. Inicializa os subsistemas e seta os bits correspondentes no EventGroup
    if (EPS.Init() == SUBSYS_OK) {
        Serial.println("[INIT] EPS OK");
        System_SetSubsystemReady(SYS_BIT_EPS_READY);
    } else {
        Serial.println("[INIT] Falha na inicialização do EPS!");
    }

    if (ADCS.Init() == SUBSYS_OK) {
        Serial.println("[INIT] ADCS OK");
        System_SetSubsystemReady(SYS_BIT_ADCS_READY);
    } else {
        Serial.println("[INIT] Falha na inicialização do ADCS!");
    }

    if (TTC.Init() == SUBSYS_OK) {
        Serial.println("[INIT] TT&C OK");
        System_SetSubsystemReady(SYS_BIT_TTC_READY);
    } else {
        Serial.println("[INIT] Falha na inicialização do TT&C!");
    }

    // 3. Aguarda até 5 segundos para a prontidão dos subsistemas críticos
    if (System_WaitAllCriticalReady(pdMS_TO_TICKS(5000))) {
        Serial.println("[ConOps] Todos os subsistemas críticos estão prontos!");
        System_SetState(STATE_MISSION);
    } else {
        Serial.println("[ConOps] AVISO: Nem todos os subsistemas responderam. Entrando em STATE_SAFE.");
        System_SetState(STATE_SAFE);
    }

    // 4. Criação das Tasks nos dois núcleos do ESP32
    
    // Core 1: Tarefas determinísticas e de tempo real
    xTaskCreatePinnedToCore(TaskTTC,  "TaskTTC",  4096, NULL, 3, &xTaskTTC_Handle,  1);
    xTaskCreatePinnedToCore(TaskADCS, "TaskADCS", 4096, NULL, 3, &xTaskADCS_Handle, 1);

    // Core 0: Tarefas de monitoramento, energia e estado de voo
    xTaskCreatePinnedToCore(TaskEPS,    "TaskEPS",    3072, NULL, 2, &xTaskEPS_Handle,    0);
    xTaskCreatePinnedToCore(TaskConOps, "TaskConOps", 3072, NULL, 1, &xTaskConOps_Handle, 0);

    Serial.println("[MAIN] Sistema em execução com FreeRTOS.");
}

void loop() {
    // O loop fica ocioso, pois a execução é mantida pelas tarefas do FreeRTOS
    vTaskDelay(pdMS_TO_TICKS(10000));
}

#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include "../eps-tc/src/eps.h"
#include "../tt-c/src/ttc.h" 
#include "esp_heap_caps.h"
#include "sd_logger.h"
#include "base_comms.h"
#include <RTClib.h>
#include "esp_task_wdt.h"
#include "conops.h"

// Sub-sistemas flags do EventGroup
#define EPS_READY       (1 << 0)
#define UART_READY      (1 << 1)
#define I2C_READY       (1 << 2)
#define HSPI_READY      (1 << 3)
#define RTC_READY       (1 << 4)
#define VSPI_READY      (1 << 5)
#define INIT_ERROR      (1 << 6)
#define MISSION_READY   (1 << 7)
#define SD_READY        (1 << 8)
#define BASE_READY      (1 << 9)

EventGroupHandle_t systemEvents;
RTC_DS3231 rtc;

constexpr BaseType_t CORE_IO = 0;
constexpr BaseType_t CORE_CONTROL = 1;

#define LED_BUILTIN 2
#define PIN_EN 32
#define PIN_BURN 33

// Variáveis de estado e Mutex removidos deste arquivo (gerenciados em conops.cpp)

// TASK: GERENCIADOR DO CONOPS
void TaskStateManager(void *parameter) {
    const EventBits_t requiredBits = EPS_READY | I2C_READY | RTC_READY | VSPI_READY;
    
    EventBits_t bits = xEventGroupWaitBits(systemEvents, requiredBits | INIT_ERROR, pdFALSE, pdFALSE, portMAX_DELAY);
    
    if (bits & INIT_ERROR) {
        Serial.println("[CONOPS] Erro de boot detectado. Entrando em SAFE MODE.");

        System_SetState(STATE_SAFE);
    } else if ((bits & requiredBits) == requiredBits) {
        Serial.println("[CONOPS] Todos os subsistemas criticos OK. Entrando em READY.");

        System_SetState(STATE_READY);
        System_SetState(STATE_MISSION);
        
        // Dispara recebedor TT&C no Core 1
        xTaskCreatePinnedToCore(TaskTTC, "TaskTTC", 4096, NULL, 3, NULL, CORE_CONTROL);
    }

    esp_task_wdt_add(NULL);

    for (;;) {
        esp_task_wdt_reset();
        
        // Leitura segura do estado do sistema
        SystemState_t st = System_GetState();

        if (st == STATE_SAFE) {
            digitalWrite(PIN_BURN, LOW);
            digitalWrite(PIN_EN, LOW);
            digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN)); // Pisca indicando SAFE
        } else if (st == STATE_MISSION) {
            digitalWrite(LED_BUILTIN, HIGH);
        }

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

// TASKS OPERACIONAIS

void TaskEPS(void *parameter) { 
    if (EPS_Init() == SUBSYSTEM_OK) {
        xEventGroupSetBits(systemEvents, EPS_READY);
    } else {
        xEventGroupSetBits(systemEvents, INIT_ERROR);
        vTaskDelete(NULL);
        return;
    }

    esp_task_wdt_add(NULL);

    for (;;) {
        esp_task_wdt_reset();

        EPS_Telemetry_t telemetryData;
        if (EPS_GetTelemetry(&telemetryData) == SUBSYSTEM_OK) {
            if (!EPS_HealthCheck()) {
                Serial.println("[ALERTA] EPS fora dos limites! Transicao para SAFE MODE.");
                // ALTERAÇÃO 4: Troca para SAFE via API
                System_SetState(STATE_SAFE);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void TaskCommunication(void *parameter) {
    if (Wire.begin(PIN_SDAitc, PIN_SCLitc, 400000)) {
        if (rtc.begin()) {
            if (rtc.lostPower()) {
                rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
            }
            xEventGroupSetBits(systemEvents, I2C_READY | RTC_READY);
        } else {
            xEventGroupSetBits(systemEvents, INIT_ERROR);
        }
    } else {
        xEventGroupSetBits(systemEvents, INIT_ERROR);
    }
    vTaskDelete(NULL);
}

void TaskSPI(void *parameter) {
    SPI.begin(PIN_SCKv, PIN_MISOv, PIN_MOSIv, PIN_CSv);
    if (TTC_Init() == SUBSYSTEM_OK) {
        xEventGroupSetBits(systemEvents, VSPI_READY);
    } else {
        xEventGroupSetBits(systemEvents, INIT_ERROR);
    }
    vTaskDelete(NULL);
}

void setup() {
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);
    pinMode(PIN_BURN, OUTPUT);
    digitalWrite(PIN_BURN, LOW);
    pinMode(PIN_EN, OUTPUT);
    digitalWrite(PIN_EN, LOW);
    
    Serial.begin(115200);

    //Inicialização do módulo de ConOps e criação do Mutex interno
    System_InitConOps();

    esp_task_wdt_init(10, true);
    esp_task_wdt_add(NULL);

    systemEvents = xEventGroupCreate();

    if (SD_Init() && SD_CreateLog() && SD_StartTask()) {
        xEventGroupSetBits(systemEvents, HSPI_READY | SD_READY);
    }
    if (Base_Init() && Base_StartTask()) {
        xEventGroupSetBits(systemEvents, UART_READY | BASE_READY);
    }

    // Criação das Tasks no Core 0 (I/O e gerenciamento)
    xTaskCreatePinnedToCore(TaskEPS, "TaskEPS", 4096, NULL, 4, NULL, CORE_IO);
    xTaskCreatePinnedToCore(TaskCommunication, "TaskCommunication", 4096, NULL, 2, NULL, CORE_IO);
    xTaskCreatePinnedToCore(TaskSPI, "TaskSPI", 4096, NULL, 2, NULL, CORE_IO);
    xTaskCreatePinnedToCore(TaskStateManager, "TaskStateManager", 4096, NULL, 3, NULL, CORE_IO);
}

void loop() {
    esp_task_wdt_reset();
    vTaskDelay(pdMS_TO_TICKS(1000));
}

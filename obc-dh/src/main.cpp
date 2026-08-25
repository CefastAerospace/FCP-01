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

// Afinidade dos subsistemas no ESP32 dual-core.
constexpr BaseType_t CORE_IO = 0;
constexpr BaseType_t CORE_CONTROL = 1;

// Pinagem
#define LED_BUILTIN 2

// Instanciação do RTC
RTC_DS3231 rtc;

// I2C - TC, EPS e ADCS
#define PIN_SDAitc 21
#define PIN_SCLitc 22

// VSPI - LoRa SX1276 TT&C 
#define PIN_SCKv 18
#define PIN_MISOv 19
#define PIN_MOSIv 23
#define PIN_CSv 5
#define PIN_RSTv 14
#define PIN_DIO0v 4

// PWM - SimpleFOC
#define PIN_PWMu 25
#define PIN_PWMv 26
#define PIN_PWMw 27
#define PIN_EN 32

// Burn wire - Gatilho
#define PIN_BURN 33

// Cada bit representa um subsistema que terminou sua inicialização.
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

// Event Group
EventGroupHandle_t systemEvents;

// Timestamp universal
void GetCurrentTimestamp(char* buffer, size_t maxLen) {
    DateTime now = rtc.now();
    snprintf(buffer, maxLen, "%04d-%02d-%02dT%02d:%02d:%02d", 
             now.year(), now.month(), now.day(), 
             now.hour(), now.minute(), now.second());
}

// TASK - SUBSISTEMA EPS (Supervisão e Monitoramento Contínuo)
void TaskEPS(void *parameter) { 
    Serial.println("[BOOT] Inicializando EPS...");
    
    // Utiliza a nova interface padronizada EPS_Init()
    if (EPS_Init() == SUBSYSTEM_OK) {
        Serial.println("[OK] EPS inicializado");
        xEventGroupSetBits(systemEvents, EPS_READY);
    } else {
        Serial.println("[ERRO] Falha na inicialização do EPS");
        xEventGroupSetBits(systemEvents, INIT_ERROR);
        vTaskDelete(NULL); // Cancela a tarefa em caso de erro crítico de boot
        return;
    }

    esp_task_wdt_add(NULL); // Registra a task no Watchdog

    // Loop operacional do EPS no Core 0
    for (;;) {
        esp_task_wdt_reset();

        // Lógica padronizada de leitura e verificação de limites
        EPS_Telemetry_t telemetryData;
        if (EPS_GetTelemetry(&telemetryData) == SUBSYSTEM_OK) {
            if (!EPS_HealthCheck()) {
                Serial.println("[ALERTA] EPS fora dos limites de segurança!");
            }
        }

        vTaskDelay(pdMS_TO_TICKS(1000)); // Frequência de monitoramento: 1Hz
    }
}

// I2C - TC, EPS e ADCS
void TaskCommunication(void *parameter) {
    if (Wire.begin(PIN_SDAitc, PIN_SCLitc, 400000)) {
        if (rtc.begin()) {
            if (rtc.lostPower()) {
                // Se a bateria do RTC descarregou, usa a data/hora da compilação como fallback
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

// TASK - INICIALIZAÇÃO DAS INTERFACES SPI E LORA
void TaskSPI(void *parameter) {
    Serial.println("[BOOT] Inicializando interfaces SPI...");
    SPI.begin(PIN_SCKv, PIN_MISOv, PIN_MOSIv, PIN_CSv);
    
    // Inicializa o driver do rádio LoRa TT&C
    if (TTC_Init()) {
        Serial.println("[OK] VSPI (LoRa SX1276) inicializado");
        xEventGroupSetBits(systemEvents, VSPI_READY);
    } else {
        Serial.println("[ERRO] Falha ao inicializar o LoRa TT&C");
        xEventGroupSetBits(systemEvents, INIT_ERROR);
    }
    vTaskDelete(NULL);
}

// TASK OPERACIONAL - PROCESSAMENTO DE COMANDOS DA GROUND STATION (CORE 1)
void TaskTTC(void *parameter) {
    Serial.println("[TASK] Loop TT&C iniciado no Core 1.");
    esp_task_wdt_add(NULL); // Registra a task no Watchdog de Hardware

    for (;;) {
        esp_task_wdt_reset(); // Alimenta o Watchdog

        // Processa a entrada de pacotes LoRa e dispara o dispatcher de telecomandos
        TTC_CheckIncomingCommands();

        vTaskDelay(pdMS_TO_TICKS(100)); // Taxa de amostragem de 10Hz
    }
}

// TASK - GERENCIADOR DE BOOT
void TaskBootManager(void *parameter) {
    const EventBits_t requiredBits = EPS_READY | I2C_READY | RTC_READY | VSPI_READY;
    EventBits_t bits = xEventGroupWaitBits(systemEvents, requiredBits | INIT_ERROR, pdFALSE, pdFALSE, portMAX_DELAY);
    
    if (bits & INIT_ERROR) {
        while (1) {
            digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
            vTaskDelay(pdMS_TO_TICKS(500));
        }
    }
    
    if ((bits & requiredBits) == requiredBits) {
        Serial.println();
        Serial.println("   OBC FCP-01");
        Serial.println("   PRONTO PARA MISSAO");
        
        EventBits_t optionalBits = xEventGroupGetBits(systemEvents);
        if ((optionalBits & SD_READY) == 0) {
            Serial.println("   AVISO: SD indisponivel");
        }
        if ((optionalBits & BASE_READY) == 0) {
            Serial.println("   AVISO: BASE indisponivel");
        }
        
        xEventGroupSetBits(systemEvents, MISSION_READY);

        // Registra o evento de boot concluído no SD Card
        char dateBuf[12], timeBuf[10];
        DateTime now = rtc.now();
        snprintf(dateBuf, sizeof(dateBuf), "%04d-%02d-%02d", now.year(), now.month(), now.day());
        snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d:%02d", now.hour(), now.minute(), now.second());
        SD_Log(dateBuf, timeBuf, LOG_SYSTEM, "OBC Boot Concluido com Sucesso");

        // Cria a tarefa de recepção contínua de comandos no Core 1 (Prioridade 3)
        xTaskCreatePinnedToCore(TaskTTC, "TaskTTC", 4096, NULL, 3, NULL, CORE_CONTROL);
    }
    vTaskDelete(NULL);
}

void setup() {
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW); // Led de inicialização do ESP
    pinMode(PIN_BURN, OUTPUT);
    digitalWrite(PIN_BURN, LOW); // Gatilho do Burn Wire desligado
    pinMode(PIN_EN, OUTPUT);
    digitalWrite(PIN_EN, LOW); // Driver do motor desligado
    
    Serial.begin(115200);

    // Inicializa o watchdog antes de criar qualquer task supervisionada.
    esp_task_wdt_init(10, true);
    esp_task_wdt_add(NULL);

    while (!Serial) {
        static const unsigned long serialWaitStart = millis();
        if (millis() - serialWaitStart >= 3000) {
            break;
        }
        delay(100);
        digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    }
    digitalWrite(LED_BUILTIN, HIGH);
    Serial.println("       OBC FCP-01 BOOT");

    systemEvents = xEventGroupCreate();
    if (systemEvents == NULL) {
        Serial.println("[FATAL] Falha ao criar Event Group!");
        while (true) {
            digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
            delay(200);
        }
    }
    Serial.println("[BOOT] Event Group criado");

    // CARTÃO SD
    if (SD_Init()) {
        if (SD_CreateLog()) {
            if (SD_StartTask()) {
                xEventGroupSetBits(systemEvents, HSPI_READY | SD_READY);
                Serial.println("[OBC] SD pronto.");
            } else {
                Serial.println("[OBC] ERRO: SD Task nao iniciou.");
            }
        } else {
            Serial.println("[OBC] ERRO: MISSION.CSV nao foi criado.");
        }
    } else {
        Serial.println("[OBC] ERRO: SD nao inicializado.");
    }

    // BASE/RPI
    if (Base_Init()) {
        if (Base_StartTask()) {
            xEventGroupSetBits(systemEvents, UART_READY | BASE_READY);
            Serial.println("[OBC] Comunicacao com a base pronta.");
        } else {
            Serial.println("[OBC] ERRO: Base Task nao iniciou.");
        }
    } else {
        Serial.println("[OBC] ERRO: UART da base nao inicializada.");
    }

    // Instanciação das tarefas de inicialização e monitoramento (Core 0)
    xTaskCreatePinnedToCore(TaskEPS, "TaskEPS", 4096, NULL, 4, NULL, CORE_IO);
    xTaskCreatePinnedToCore(TaskCommunication, "TaskCommunication", 4096, NULL, 2, NULL, CORE_IO);
    xTaskCreatePinnedToCore(TaskSPI, "TaskSPI", 4096, NULL, 2, NULL, CORE_IO);
    xTaskCreatePinnedToCore(TaskBootManager, "TaskBootManager", 4096, NULL, 3, NULL, CORE_IO);
    
    Serial.println("[BOOT] Tasks de inicialização criadas");
}

void loop() {
    esp_task_wdt_reset();
    vTaskDelay(pdMS_TO_TICKS(1000));
}
#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include "../eps-tc/src/eps.h"
#include "../tt-c/src/ttc.h"
#include "../common/payload.h" // Inclusão da interface da Payload
#include "esp_heap_caps.h"
#include "sd_logger.h"
#include "base_comms.h"
#include <RTClib.h>
#include "esp_task_wdt.h"
#include "subsystem_interface.h"
#include "conops.h"

// Afinidade dos subsistemas no ESP32 dual-core
constexpr BaseType_t CORE_IO = 0;
constexpr BaseType_t CORE_CONTROL = 1;

// Bits de prontidão do EventGroup
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

// Handles das Tarefas do FreeRTOS
TaskHandle_t xTaskTTC_Handle     = NULL;
TaskHandle_t xTaskEPS_Handle     = NULL;
TaskHandle_t xTaskADCS_Handle    = NULL;
TaskHandle_t xTaskConOps_Handle  = NULL;
TaskHandle_t xTaskPayload_Handle = NULL; // Handle da Task da Payload

// ==========================================
// TAREFAS FREERTOS (TASKS)
// ==========================================

/**
 * @brief Tarefa do TT&C (Telecomunicações) - Core 1
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
 */
void TaskEPS(void *pvParameters) {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(500); // 2 Hz

    for (;;) {
        EPS.TaskUpdate();

        if (!EPS.HealthCheck()) {
            Serial.println("[EPS] ALERTA: Tensão baixa ou superaquecimento detectado!");
        }

        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}

/**
 * @brief Tarefa do ConOps (Supervisão de Estados) - Core 0
 */
void TaskConOps(void *pvParameters) {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(1000); // 1 Hz

    for (;;) {
        if (!EPS.HealthCheck() && System_GetState() == STATE_MISSION) {
            System_SetState(STATE_SAFE);
            Serial.println("[ConOps] Forçando STATE_SAFE devido a falha no EPS!");
        }

        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}

/**
 * @brief Tarefa da Payload (Raspberry Pi Zero W / ADS-B) - Core 0
 * Processa o protocolo UART binário e gerencia o link de comunicação.
 */
void TaskPayloadWrapper(void *pvParameters) {
    // Delega a execução contínua para o gerenciador interno da Payload
    Payload_Task(pvParameters);
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

    if (Payload_Init() == SUBSYS_OK) {
        Serial.println("[INIT] Payload (UART RPi) OK");
        System_SetSubsystemReady(SYS_BIT_PAYLOAD_READY);
    } else {
        Serial.println("[INIT] Falha na inicialização da Payload!");
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
    
    // Core 1 (CORE_CONTROL): Tarefas determinísticas e de tempo real
    xTaskCreatePinnedToCore(TaskTTC,   "TaskTTC",   4096, NULL, 3, &xTaskTTC_Handle,   CORE_CONTROL);
    xTaskCreatePinnedToCore(TaskADCS,  "TaskADCS",  4096, NULL, 3, &xTaskADCS_Handle,  CORE_CONTROL);

    // Core 0 (CORE_IO): Tarefas de monitoramento, energia, estado e comunicação UART
    xTaskCreatePinnedToCore(TaskEPS,            "TaskEPS",     3072, NULL, 2, &xTaskEPS_Handle,     CORE_IO);
    xTaskCreatePinnedToCore(TaskConOps,         "TaskConOps",  3072, NULL, 1, &xTaskConOps_Handle,  CORE_IO);
    xTaskCreatePinnedToCore(TaskPayloadWrapper, "TaskPayload", 4096, NULL, 2, &xTaskPayload_Handle, CORE_IO);

    Serial.println("[MAIN] Sistema em execução com FreeRTOS.");
}

void loop() {
    if (Serial.available() > 0) {
        String input = Serial.readStringUntil('\n');
        input.trim();

        if (input.length() == 0) return;

        // --- COMANDOS DO MOTOR ADCS ---
        if (input.startsWith("rpm ") || input.startsWith("RPM ")) {
            float target_rpm = input.substring(4).toFloat();

            SubsystemCommand_t cmd;
            cmd.command_id = 0x22; // CMD_ADCS_SET_RPM
            cmd.payload_len = sizeof(float);
            memcpy(cmd.payload, &target_rpm, sizeof(float));

            SubsystemStatus_t status = ADCS.HandleCommand(cmd);

            if (status == SUBSYS_OK) {
                Serial.printf("\n[MANUAL] ADCS: Alvo de velocidade atualizado para %.1f RPM\n", target_rpm);
            } else {
                Serial.println("\n[MANUAL] Erro ao enviar comando para o ADCS!");
            }
        }
        else if (input.equalsIgnoreCase("stop")) {
            SubsystemCommand_t cmd;
            cmd.command_id = 0x21; // CMD_ADCS_STOP
            cmd.payload_len = 0;
            ADCS.HandleCommand(cmd);
            Serial.println("\n[MANUAL] ADCS: Motor parado (IDLE).");
        }
        else if (input.equalsIgnoreCase("estop")) {
            SubsystemCommand_t cmd;
            cmd.command_id = 0xFF; // Emergency Stop
            cmd.payload_len = 0;
            ADCS.HandleCommand(cmd);
            Serial.println("\n[MANUAL] ADCS: PARADA DE EMERGÊNCIA ACIONADA!");
        }

        // --- COMANDOS DA PAYLOAD (RASPBERRY PI VIA UART) ---
        else if (input.equalsIgnoreCase("payload start")) {
            Payload_HandleCommand(0x10, NULL, 0); // 0x10: PAYLOAD_CMD_START_ACQ
            Serial.println("\n[MANUAL] Payload: Comando de início de coleta enviado via UART.");
        }
        else if (input.equalsIgnoreCase("payload stop")) {
            Payload_HandleCommand(0x11, NULL, 0); // 0x11: PAYLOAD_CMD_STOP_ACQ
            Serial.println("\n[MANUAL] Payload: Comando de parada enviado via UART.");
        }
        else if (input.equalsIgnoreCase("payload reboot")) {
            Payload_HandleCommand(0x1F, NULL, 0); // 0x1F: PAYLOAD_CMD_SHUTDOWN
            Serial.println("\n[MANUAL] Payload: Comando de reinicialização enviado via UART.");
        }

        // --- COMANDOS DA MÁQUINA DE ESTADOS (CONOPS) ---
        else if (input == "1") {
            System_SetState(STATE_MISSION);
            Serial.println("\n[MANUAL] Estado alterado para: STATE_MISSION");
        }
        else if (input == "2") {
            System_SetState(STATE_SAFE);
            Serial.println("\n[MANUAL] Estado alterado para: STATE_SAFE");
        }
        else if (input == "3") {
            System_SetState(STATE_BOOT_CHECK);
            Serial.println("\n[MANUAL] Estado alterado para: STATE_BOOT_CHECK");
        }

        // --- REBOOT E DIAGNÓSTICO DO SISTEMA ---
        else if (input.equalsIgnoreCase("r")) {
            Serial.println("\n[MANUAL] Reiniciando OBC (ESP32)...");
            Serial.flush();
            ESP.restart();
        }
        else if (input.equalsIgnoreCase("s")) {
            Payload_Telemetry_t p_telemetry;
            Payload_GetTelemetry(&p_telemetry);

            Serial.println("\n=== STATUS ATUAL DO SATÉLITE ===");
            Serial.printf("Estado ConOps: %d\n", System_GetState());
            Serial.printf("Bateria: %.2f V | Temperatura OBC: %.1f °C\n", EPS.GetBatteryVoltage(), 24.5f);
            Serial.printf("ADCS Target RPM: %.1f | Current RPM: %.1f\n", ADCS.GetTargetRPM(), ADCS.GetCurrentRPM());
            Serial.printf("Payload RPi Conectada: %s | Pacotes ADS-B: %u | Temp RPi: %.1f °C\n",
                          p_telemetry.is_pi_responsive ? "SIM" : "NAO",
                          p_telemetry.adsb_messages_received,
                          p_telemetry.pi_temperature_c);
        }

        // --- MENU DE AJUDA ---
        else if (input.equalsIgnoreCase("h") || input.equalsIgnoreCase("help")) {
            Serial.println("\n=== COMANDOS DISPONÍVEIS VIA SERIAL ===");
            Serial.println("  1              -> Forçar STATE_MISSION");
            Serial.println("  2              -> Forçar STATE_SAFE");
            Serial.println("  3              -> Forçar STATE_BOOT_CHECK");
            Serial.println("  rpm <valor>    -> Define velocidade do motor ADCS (ex: rpm 1500 ou rpm -500)");
            Serial.println("  stop           -> Para o motor do ADCS");
            Serial.println("  estop          -> Dispara parada de emergência do ADCS");
            Serial.println("  payload start  -> Inicia aquisição de dados na RPi Zero W");
            Serial.println("  payload stop   -> Interrompe aquisição na RPi Zero W");
            Serial.println("  payload reboot -> Executa shutdown/reboot na RPi Zero W");
            Serial.println("  s              -> Imprime o status da telemetria e ConOps");
            Serial.println("  r              -> Reinicia o microcontrolador (Reboot OBC)");
        }
        else {
            Serial.println("\nComando desconhecido. Digite 'help' para listar as opções.");
        }
    }

    vTaskDelay(pdMS_TO_TICKS(50));
}
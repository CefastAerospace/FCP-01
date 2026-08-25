#include "sd_logger.h"
#include <SPI.h>
#include <SD.h>
#include "esp_task_wdt.h"

static constexpr BaseType_t CORE_IO = 0;

// SPI
SPIClass SD_SPI(HSPI);

// VARIÁVEIS INTERNAS
static bool sdAvailable = false;
static QueueHandle_t xQueueLog = NULL;
static TaskHandle_t xSDTask = NULL;

// CONVERSÃO DO TIPO
static const char* LogTypeToString(LogType type) {
    switch (type) {
        case LOG_AIRCRAFT:  return "AIRCRAFT";
        case LOG_TELEMETRY: return "TELEMETRY";
        case LOG_EVENT:     return "EVENT";
        case LOG_SYSTEM:    return "SYSTEM";
        default:            return "UNKNOWN";
    }
}

// INICIALIZAÇÃO DO SD
bool SD_Init() {
    Serial.println("[SD] Inicializando...");

    SD_SPI.begin(PIN_SCKh, PIN_MISOh, PIN_MOSIh, PIN_CSh);

    if (!SD.begin(PIN_CSh, SD_SPI, SD_FREQUENCY)) {
        Serial.println("[SD] ERRO: falha ao inicializar!");
        sdAvailable = false;
        return false;
    }

    uint8_t cardType = SD.cardType();
    if (cardType == CARD_NONE) {
        Serial.println("[SD] ERRO: nenhum cartao detectado!");
        sdAvailable = false;
        return false;
    }

    sdAvailable = true;
    Serial.println("[SD] Cartao inicializado com sucesso!");
    return true;
}

// CRIAÇÃO DO LOG
bool SD_CreateLog() {
    if (!sdAvailable) return false;

    if (SD.exists(SD_LOG_FILE)) {
        return true;
    }

    File file = SD.open(SD_LOG_FILE, FILE_WRITE);
    if (!file) {
        Serial.println("[SD] ERRO ao criar MISSION.CSV!");
        return false;
    }

    file.println("DATE,TIME,TYPE,PAYLOAD_DATA");
    file.close();
    Serial.println("[SD] MISSION.CSV criado!");
    return true;
}

// SD TASK (CONSUMIDOR DA FILA)
static void SD_Task(void* parameter) {
    LogPacket packet;
    uint8_t syncCounter = 0;
    esp_task_wdt_add(NULL);

    File file;

    while (true) {
        esp_task_wdt_reset();

        if (xQueueReceive(xQueueLog, &packet, pdMS_TO_TICKS(100)) == pdTRUE) {
            if (!sdAvailable) {
                SD_Init();
            }

            if (sdAvailable) {
                file = SD.open(SD_LOG_FILE, FILE_APPEND);
                if (file) {
                    // Escreve a linha no formato CSV: DATE,TIME,TYPE,PAYLOAD_DATA
                    file.printf("%s,%s,%s,%s\n", 
                                packet.data, 
                                packet.time, 
                                LogTypeToString(packet.type), 
                                packet.payload);

                    syncCounter++;
                    if (syncCounter >= 5) {
                        file.flush(); // Garante a gravação no hardware
                        syncCounter = 0;
                    }
                    file.close(); // Fecha o arquivo para evitar corrupção em caso de brownout
                } else {
                    Serial.println("[SD] Erro ao abrir arquivo para escrita.");
                    sdAvailable = false;
                }
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

// CRIAÇÃO DA TASK
bool SD_StartTask() {
    if (!sdAvailable) return false;
    if (xQueueLog != NULL) return false;

    xQueueLog = xQueueCreate(LOG_QUEUE_LENGTH, sizeof(LogPacket));
    if (xQueueLog == NULL) return false;

    BaseType_t result = xTaskCreatePinnedToCore(
        SD_Task, "SD_Task", 4096, NULL, 2, &xSDTask, CORE_IO
    );

    if (result != pdPASS) {
        vQueueDelete(xQueueLog);
        xQueueLog = NULL;
        return false;
    }

    return true;
}

// LOG DIRETO / PRODUTOR
bool SD_Log(const char* data, const char* time, LogType type, const char* dataPacket) {
    LogPacket packet = {};
    snprintf(packet.data, sizeof(packet.data), "%s", data != nullptr ? data : "");
    snprintf(packet.time, sizeof(packet.time), "%s", time != nullptr ? time : "");
    snprintf(packet.payload, sizeof(packet.payload), "%s", dataPacket != nullptr ? dataPacket : "");
    packet.type = type;

    return SD_SendLog(packet);
}

// ENVIO PARA A QUEUE
bool SD_SendLog(const LogPacket& packet) {
    if (xQueueLog == NULL) return false;

    if (packet.type == LOG_EVENT || packet.type == LOG_SYSTEM) {
        return (xQueueSendToFront(xQueueLog, &packet, pdMS_TO_TICKS(100)) == pdTRUE);
    }
    return (xQueueSend(xQueueLog, &packet, pdMS_TO_TICKS(100)) == pdTRUE);
}

bool SD_IsAvailable() { return sdAvailable; }

uint64_t SD_GetCardSizeMB() {
    return sdAvailable ? (SD.cardSize() / (1024 * 1024)) : 0;
}

uint64_t SD_GetFreeSpaceMB() {
    if (!sdAvailable) return 0;
    uint64_t total = SD.totalBytes();
    uint64_t used = SD.usedBytes();
    return (used >= total) ? 0 : ((total - used) / (1024 * 1024));
}
#include "sd_logger.h"

#include <SPI.h>
#include <SD.h>

// SPI
SPIClass SD_SPI(HSPI);

// VARIÁVEIS INTERNAS
static bool sdAvailable = false;
static QueueHandle_t xQueueLog = NULL;
static TaskHandle_t xSDTask = NULL;

// CONVERSÃO DO TIPO
static const char* LogTypeToString(LogType type)
{
    switch (type)
    {
        case LOG_AIRCRAFT:
            return "AIRCRAFT";

        case LOG_TELEMETRY:
            return "TELEMETRY";

        case LOG_EVENT:
            return "EVENT";

        case LOG_SYSTEM:
            return "SYSTEM";

        default:
            return "UNKNOWN";
    }
}

// INICIALIZAÇÃO DO SD
bool SD_Init()
{
    Serial.println("[SD] Inicializando...");

    SD_SPI.begin(
        PIN_SCKh,
        PIN_MISOh,
        PIN_MOSIh,
        PIN_CSh
    );

    if (!SD.begin(PIN_CSh, SD_SPI, SD_FREQUENCY))
    {
        Serial.println("[SD] ERRO: falha ao inicializar!");
        sdAvailable = false;
        return false;
    }

    uint8_t cardType = SD.cardType();

    if (cardType == CARD_NONE)
    {
        Serial.println("[SD] ERRO: nenhum cartao detectado!");
        sdAvailable = false;
        return false;
    }

    sdAvailable = true;

    Serial.println("[SD] Cartao inicializado!");

    Serial.print("[SD] Capacidade: ");
    Serial.print(SD_GetCardSizeMB());
    Serial.println(" MB");

    Serial.print("[SD] Espaco usado: ");
    Serial.print(SD.usedBytes() / (1024 * 1024));
    Serial.println(" MB");

    return true;
}

// CRIAÇÃO DO LOG

bool SD_CreateLog()
{
    if (!sdAvailable)
    {
        Serial.println("[SD] Cartao indisponivel!");
        return false;
    }

    if (SD.exists(SD_LOG_FILE))
    {
        Serial.println("[SD] MISSION.CSV ja existe.");
        return true;
    }

    File file = SD.open(SD_LOG_FILE, FILE_WRITE);

    if (!file)
    {
        Serial.println("[SD] ERRO ao criar MISSION.CSV!");
        return false;
    }

    file.println(""DATE,TIME,TYPE,PAYLOAD_DATA"");
    file.close();
    Serial.println("[SD] MISSION.CSV criado!");
    return true;
}

// ESCRITA NO SD

bool SD_Log(
    const char* data,
    const char* time,
    LogType type,
    const char* dataPacket
)
{
    if (!sdAvailable)
    {
        return false;
    }

    File file = SD.open(SD_LOG_FILE, FILE_APPEND);

    if (!file)
    {
        Serial.println("[SD] ERRO ao abrir MISSION.CSV!");
        return false;
    }

    file.print(data);
    file.print(",");

    file.print(time);
    file.print(",");

    file.print(LogTypeToString(type));
    file.print(",");

    file.println(dataPacket);

    file.close();

    return true;
}

// SD TASK
static void SD_Task(void* parameter) {
    LogPacket packet;
    uint8_t syncCounter = 0;
    esp_task_wdt_add(NULL); // Adiciona a task ao watchdog

    // Abre o arquivo uma vez antes do loop infinito
    File file = SD.open(SD_LOG_FILE, FILE_APPEND);

    while (true) {
        esp_task_wdt_reset(); // Alimenta o watchdog
        if (xQueueReceive(xQueueLog, &packet, pdMS_TO_TICKS(100)) == pdTRUE) {
            if (sdAvailable && file) {
                file.print(packet.data); file.print(",");
                file.print(packet.time); file.print(",");
                file.print(packet.type); file.print(",");
                file.println(packet.payload);

                syncCounter++;
                if (syncCounter >= 5) {
                    file.flush(); // Salva fisicamente no cartão sem fechar o arquivo
                    syncCounter = 0;
                }
            }
        }
    }
}
// CRIAÇÃO DA TASK
bool SD_StartTask()
{
    if (!sdAvailable)
    {
        Serial.println(
            "[SD TASK] SD indisponivel!"
        );

        return false;
    }

    if (xQueueLog != NULL)
    {
        Serial.println(
            "[SD TASK] Queue ja existe!"
        );

        return false;
    }

    xQueueLog = xQueueCreate(
        LOG_QUEUE_LENGTH,
        sizeof(LogPacket)
    );

    if (xQueueLog == NULL)
    {
        Serial.println(
            "[SD TASK] ERRO ao criar Queue!"
        );

        return false;
    }

    BaseType_t result = xTaskCreatePinnedToCore(
        SD_Task,
        "SD_Task",
        4096,
        NULL,
        2,
        &xSDTask,
        1
    );

    if (result != pdPASS)
    {
        Serial.println(
            "[SD TASK] ERRO ao criar Task!"
        );

        vQueueDelete(xQueueLog);

        xQueueLog = NULL;

        return false;
    }

    Serial.println(
        "[SD TASK] Task criada com sucesso!"
    );

    return true;
}

// ENVIO PARA A QUEUE
bool SD_SendLog(const LogPacket& packet) {
    if (xQueueLog == NULL) return false;

    // Se for um evento crítico ou sistema, joga direto para o início da fila
    if (packet.type == LOG_EVENT || packet.type == LOG_SYSTEM) {
        return (xQueueSendToFront(xQueueLog, &packet, pdMS_TO_TICKS(100)) == pdTRUE);
    }
    // Dados normais entram no fim da fila (FIFO)
    return (xQueueSend(xQueueLog, &packet, pdMS_TO_TICKS(100)) == pdTRUE);
}

// STATUS
bool SD_IsAvailable()
{
    return sdAvailable;
}

// TAMANHO DO CARTÃO
uint64_t SD_GetCardSizeMB()
{
    if (!sdAvailable)
    {
        return 0;
    }

    return SD.cardSize() / (1024 * 1024);
}

// ESPAÇO LIVRE
uint64_t SD_GetFreeSpaceMB()
{
    if (!sdAvailable)
    {
        return 0;
    }

    uint64_t total = SD.totalBytes();
    uint64_t used = SD.usedBytes();

    if (used >= total)
    {
        return 0;
    }

    return (total - used) / (1024 * 1024);
}

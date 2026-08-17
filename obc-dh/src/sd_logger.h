#ifndef SD_LOGGER_H
#define SD_LOGGER_H

#include <Arduino.h>

// PINAGEM - SD CARD / HSPI

#define PIN_SCKh   15
#define PIN_MISOh  35
#define PIN_MOSIh  12
#define PIN_CSh    13

// CONFIGURAÇÕES

#define SD_FREQUENCY 10000000

#define SD_LOG_FILE "/MISSION.CSV"

// FILA DE LOG

#define LOG_QUEUE_LENGTH 20

// TIPOS DE LOG

typedef enum
{
    LOG_AIRCRAFT,
    LOG_TELEMETRY,
    LOG_EVENT,
    LOG_SYSTEM

} LogType;

// ESTRUTURA DO PACOTE

typedef struct
{
    LogType type;

    char data[11];      // YYYY-MM-DD
    char time[16];      // HH:MM:SS.mmm

    char payload[256];

} LogPacket;

// INICIALIZAÇÃO

bool SD_Init();

bool SD_CreateLog();

// LOG DIRETO

bool SD_Log(
    const char* data,
    const char* time,
    LogType type,
    const char* dataPacket
);

// STATUS

bool SD_IsAvailable();

uint64_t SD_GetCardSizeMB();

uint64_t SD_GetFreeSpaceMB();

// FREERTOS

// Cria a Queue e a SD Task
bool SD_StartTask();

// Envia um pacote para a SD Task
bool SD_SendLog(const LogPacket& packet);

#endif

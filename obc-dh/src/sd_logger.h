#ifndef SD_LOGGER_H
#define SD_LOGGER_H

#include <Arduino.h>

// PINAGEM - SD CARD / HSPI
#define PIN_SCKh   15
#define PIN_MISOh  35
#define PIN_MOSIh  12
#define PIN_CSh    13

// CONFIGURAÇÕES DO SD

// Frequência inicial do barramento SPI do SD
#define SD_FREQUENCY 10000000

// Arquivo principal de log da missão
#define SD_LOG_FILE "/MISSION.CSV"

// TIPOS DE DADOS DO LOG
typedef enum
{
    LOG_AIRCRAFT,
    LOG_TELEMETRY,
    LOG_EVENT,
    LOG_SYSTEM

} LogType;

// INICIALIZAÇÃO

// Inicializa o barramento SPI e o cartão SD
// Retorna true se o SD estiver funcionando
bool SD_Init();

// ARQUIVO DE LOG

// Cria o arquivo de log caso ele ainda não exista
// Também cria o cabeçalho do arquivo CSV
bool SD_CreateLog();

// GRAVAÇÃO DE LOG

// Grava uma entrada no arquivo de log
//
// data       -> data do registro
// time       -> hora do registro
// type       -> tipo do registro
// dataPacket -> dados que serão armazenados
//
// Retorna true se a gravação for realizada com sucesso
bool SD_Log(
    const char* data,
    const char* time,
    LogType type,
    const char* dataPacket
);

// STATUS DO CARTÃO

// Verifica se o cartão SD está disponível
bool SD_IsAvailable();

// Retorna o espaço total do cartão em MB
uint64_t SD_GetCardSizeMB();

// Retorna o espaço disponível do cartão em MB
uint64_t SD_GetFreeSpaceMB();

#endif

#include "sd_logger.h"

#include <SPI.h>
#include <SD.h>

// OBJETO SPI DO SD CARD

SPIClass SD_SPI(HSPI);

// VARIÁVEIS INTERNAS

// Indica se o cartão SD foi inicializado corretamente
static bool sdAvailable = false;

// FUNÇÃO INTERNA

// Converte o tipo de log para texto
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

    // Inicializa o barramento HSPI
    SD_SPI.begin(
        PIN_SCKh,
        PIN_MISOh,
        PIN_MOSIh,
        PIN_CSh
    );

    // Inicializa o cartão SD
    if (!SD.begin(PIN_CSh, SD_SPI, SD_FREQUENCY))
    {
        Serial.println("[SD] ERRO: falha ao inicializar o cartao!");
        sdAvailable = false;
        return false;
    }

    // Verifica se existe um cartão
    uint8_t cardType = SD.cardType();

    if (cardType == CARD_NONE)
    {
        Serial.println("[SD] ERRO: nenhum cartao detectado!");
        sdAvailable = false;
        return false;
    }

    sdAvailable = true;
    Serial.println("[SD] Cartao inicializado com sucesso!");

    // Mostra o tipo do cartão
    Serial.print("[SD] Tipo: ");

    switch (cardType)
    {
        case CARD_MMC:
            Serial.println("MMC");
            break;

        case CARD_SD:
            Serial.println("SDSC");
            break;

        case CARD_SDHC:
            Serial.println("SDHC");
            break;

        case CARD_UNKNOWN:
            Serial.println("Desconhecido");
            break;

        default:
            Serial.println("Outro");
            break;
    }

    // Mostra capacidade
    Serial.print("[SD] Capacidade: ");
    Serial.print(SD_GetCardSizeMB());
    Serial.println(" MB");

    // Mostra espaço disponível
    Serial.print("[SD] Espaco livre: ");
    Serial.print(SD_GetFreeSpaceMB());
    Serial.println(" MB");

    return true;
}

// CRIAÇÃO DO ARQUIVO DE LOG

bool SD_CreateLog()
{
    if (!sdAvailable)
    {
        Serial.println("[SD] ERRO: cartao nao esta disponivel!");
        return false;
    }

    // Se o arquivo já existe, não recria
    if (SD.exists(SD_LOG_FILE))
    {
        Serial.println("[SD] MISSION.CSV ja existe.");
        return true;
    }

    // Cria o arquivo
    File file = SD.open(SD_LOG_FILE, FILE_WRITE);
    if (!file)
    {
        Serial.println("[SD] ERRO: nao foi possivel criar MISSION.CSV!");
        return false;
    }

    // Cabeçalho do CSV
    file.println("DATA,HORA,TIPO,DADOS");
    file.close();
    Serial.println("[SD] MISSION.CSV criado com sucesso!");
    return true;
}

// GRAVAÇÃO DE LOG

bool SD_Log(
    const char* data,
    const char* time,
    LogType type,
    const char* dataPacket
)
{
    if (!sdAvailable)
    {
        Serial.println("[SD] ERRO: tentativa de gravar sem SD!");
        return false;
    }

    // Abre o arquivo para adicionar dados no final
    File file = SD.open(SD_LOG_FILE, FILE_APPEND);

    if (!file)
    {
        Serial.println("[SD] ERRO: nao foi possivel abrir MISSION.CSV!");
        return false;
    }

    // Data
    file.print(data);
    file.print(",");

    // Hora
    file.print(time);
    file.print(",");

    // Tipo
    file.print(LogTypeToString(type));
    file.print(",");

    // Dados
    file.println(dataPacket);

    // Fecha o arquivo
    file.close();
    return true;
}

// VERIFICAÇÃO DO SD

bool SD_IsAvailable()
{
    return sdAvailable;
}

// TAMANHO TOTAL DO CARTÃO

uint64_t SD_GetCardSizeMB()
{
    if (!sdAvailable)
    {
        return 0;
    }

    return SD.cardSize() / (1024 * 1024);
}

// ESPAÇO DISPONÍVEL

uint64_t SD_GetFreeSpaceMB()
{
    if (!sdAvailable)
    {
        return 0;
    }

    return SD.totalBytes() / (1024 * 1024)
         - SD.usedBytes() / (1024 * 1024);
}

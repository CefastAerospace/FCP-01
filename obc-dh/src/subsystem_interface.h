#ifndef SUBSYSTEM_INTERFACE_H
#define SUBSYSTEM_INTERFACE_H
#include <Arduino.h>

// Status padronizado retornado pelas operações de qualquer subsistema
typedef enum {
    SUBSYS_OK = 0,             // Operação realizada com sucesso
    SUBSYS_ERR_INIT_FAILED,    // Falha na inicialização do hardware
    SUBSYS_ERR_TIMEOUT,        // Timeout em comunicação I2C/SPI
    SUBSYS_ERR_PARAM_INVALID,  // Parâmetro do comando fora dos limites
    SUBSYS_ERR_HARDWARE_FAULT, // Falha crítica nos sensores/atuadores
    SUBSYS_ERR_NOT_READY,      // Subsistema ainda não inicializado
    SUBSYS_ERR_UNKNOWN         // Erro desconhecido
} SubsystemStatus_t;

// Estrutura genérica de telecomando enviada aos subsistemas
typedef struct {
    uint8_t command_id;        // Identificador único do comando
    uint8_t payload[16];       // Argumentos do comando (até 16 bytes)
    uint8_t payload_len;       // Tamanho real dos dados do payload
} SubsystemCommand_t;

// Classe Interface Abstrata Pura
class ISubsystem {
public:
    virtual ~ISubsystem() {}

    // 1. Configuração inicial de pinos, registradores e periféricos
    virtual SubsystemStatus_t Init() = 0;

    // 2. Função de atualização periódica chamada dentro da Task FreeRTOS
    virtual void TaskUpdate() = 0;

    // 3. Serialização da telemetria para envio via LoRa ou log no SD
    virtual SubsystemStatus_t GetTelemetry(uint8_t *buffer, size_t max_len, size_t *out_len) = 0;

    // 4. Processamento de comandos recebidos da Ground Station
    virtual SubsystemStatus_t HandleCommand(const SubsystemCommand_t &cmd) = 0;

    // 5. Autodiagnóstico de saúde para o ConOps acionar SAFE MODE se necessário
    virtual bool HealthCheck() = 0;

    // 6. Retorna o nome amigável do subsistema para debug e logs
    virtual const char* GetName() = 0;
};

#endif // SUBSYSTEM_INTERFACE_H

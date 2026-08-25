#ifndef TTC_H
#define TTC_H

#include <Arduino.h>
#include "../../common/interfaces/subsystem_types.h"

// Status de resposta dos Telecomandos (ACK/NACK)
typedef enum : uint8_t {
    ACK_OK               = 0x00,
    ERR_CRC              = 0x01,
    ERR_INVALID_CMD      = 0x02,
    ERR_INVALID_PARAM    = 0x03,
    ERR_STATE_REJECTED   = 0x04
} CmdStatus_t;

// Identificadores de Telecomandos (Opcodes de Entrada)
typedef enum : uint8_t {
    CMD_PING           = 0x01,
    CMD_SET_MODE       = 0x02,
    CMD_START_MISSION  = 0x03,
    CMD_STOP_MISSION   = 0x04,
    CMD_GET_STATUS     = 0x05,
    CMD_SET_TIME       = 0x10,
    CMD_ADCS_START     = 0x20,
    CMD_ADCS_STOP      = 0x21,
    CMD_DEPLOY_ANTENNA = 0x30,
    CMD_REQUEST_LOG    = 0x40,
    CMD_RESET_OBC      = 0xFF
} CommandID_t;

// Estrutura de dados do pacote de telemetria para envio via LoRa (TM)
typedef struct {
    uint32_t timestamp;
    float voltage;
    float current;
    float temp;
    uint8_t systemStatus;
} PacketTelemetry_t;

// Estrutura genérica e compacta do Telecomando vindo da Ground Station (TC)
typedef struct __attribute__((packed)) {
    uint16_t sequence_id;   // Contador sequencial anti-replay
    uint8_t  command_id;    // Opcode do comando (CommandID_t)
    uint8_t  flags;         // Bit 0: Requisição de ACK
    uint32_t timestamp;     // Timestamp da emissão ou execução
    uint8_t  arguments[8];  // Buffer fixo para parâmetros variáveis
    uint16_t checksum;      // CRC16 de integridade
} TelecommandPacket_t;

// Estrutura de Telemetria e diagnóstico do próprio módulo TT&C
typedef struct {
    int16_t last_rssi;           // Intensidade do sinal do último pacote (dBm)
    float last_snr;              // Relação sinal-ruído (dB)
    uint32_t rx_packets_count;   // Pacotes de TC recebidos com sucesso
    uint32_t tx_packets_count;   // Pacotes de TM transmitidos com sucesso
    uint32_t rx_errors_count;    // Erros de CRC ou pacotes inválidos
    bool is_transmitting;        // Estado atual do rádio
} TTC_Telemetry_t;

// --- Interface Padrão do Módulo TT&C ---

/**
 * @brief Inicializa o transceptor LoRa via SPI, parâmetros de RF e interrupções.
 * @return SUBSYSTEM_OK se o rádio for inicializado corretamente.
 */
SubsystemStatus_t TTC_Init(void);

/**
 * @brief Tarefa principal do TT&C executada no FreeRTOS (escuta contínua).
 * @param pvParameters Parâmetros da task.
 */
void TTC_Task(void *pvParameters);

/**
 * @brief Obtém as métricas e diagnósticos do link RF.
 * @param out_telemetry Ponteiro para a estrutura de destino.
 * @return SUBSYSTEM_OK se as métricas forem válidas.
 */
SubsystemStatus_t TTC_GetTelemetry(TTC_Telemetry_t *out_telemetry);

/**
 * @brief Processa comandos de reconfiguração interna do próprio rádio.
 * @param command_id Identificador do comando.
 * @param payload Dados do comando.
 * @param length Tamanho do payload em bytes.
 * @return SUBSYSTEM_OK se processado com sucesso.
 */
SubsystemStatus_t TTC_HandleCommand(uint8_t command_id, const uint8_t *payload, uint16_t length);

/**
 * @brief Verifica o estado de saúde do rádio LoRa via SPI.
 * @return true se o rádio responder adequadamente, false em caso de falha.
 */
bool TTC_HealthCheck(void);

// --- Funções Específicas do Link de Telecomando e Telemetria ---

/**
 * @brief Transmite um pacote de telemetria (TM) para a Estação Solo.
 * @param packet Estrutura do pacote de telemetria.
 * @return true se enviado com sucesso, false caso contrário.
 */
bool TTC_SendTelemetry(const PacketTelemetry_t &packet);

/**
 * @brief Leitura e despacho de comandos recebidos via rádio.
 * @return true se um pacote válido foi processado.
 */
bool TTC_CheckIncomingCommands(void);

/**
 * @brief Decodifica e roteia o pacote de TC recebido para o subsistema de destino.
 * @param pkt Referência do pacote de telecomando decodificado.
 */
void TTC_ProcessPacket(const TelecommandPacket_t &pkt);

/**
 * @brief Envia uma resposta de confirmação/erro (ACK/NACK) para a Ground Station.
 * @param sequence_id ID de sequência do pacote original.
 * @param command_id Opcode do comando associado.
 * @param status Código de resposta (CmdStatus_t).
 */
void TTC_SendACK(uint16_t sequence_id, uint8_t command_id, CmdStatus_t status);

/**
 * @brief Calcula o CRC16 para verificação de integridade dos pacotes RF.
 * @param data Ponteiro para o buffer de dados.
 * @param len Tamanho dos dados em bytes.
 * @return Valor do checksum CRC16.
 */
uint16_t TTC_CalculateCRC16(const uint8_t *data, size_t len);

#endif // TTC_H
#ifndef PAYLOAD_H
#define PAYLOAD_H

#include <Arduino.h>
#include "../../common/interfaces/subsystem_types.h"

// Estrutura de Telemetria e Diagnóstico da Payload (Raspberry Pi Zero W / ADS-B)
typedef struct {
    uint32_t adsb_messages_received; // Total de mensagens ADS-B processadas
    uint32_t last_sync_timestamp;    // Timestamp do último pacote válido
    bool is_pi_responsive;          // Estado do link de comunicação UART
    float pi_temperature_c;          // Temperatura reportada pela Raspberry Pi
} Payload_Telemetry_t;

// --- Interface Padrão do Módulo Payload ---

/**
 * @brief Inicializa a interface UART com a Raspberry Pi Zero W.
 * @return SUBSYSTEM_OK se a comunicação for estabelecida.
 */
SubsystemStatus_t Payload_Init(void);

/**
 * @brief Tarefa FreeRTOS para gerenciamento contínuo da UART da Payload.
 * @param pvParameters Parâmetros de inicialização da task.
 */
void Payload_Task(void *pvParameters);

/**
 * @brief Obtém a telemetria e o estado operacional da Payload.
 * @param out_telemetry Ponteiro para a estrutura onde os dados serão gravados.
 * @return SUBSYSTEM_OK se as informações forem válidas.
 */
SubsystemStatus_t Payload_GetTelemetry(Payload_Telemetry_t *out_telemetry);

/**
 * @brief Trata telecomandos direcionados à Payload (ex: reiniciar Pi, alterar taxa de envio).
 * @param command_id Identificador do comando.
 * @param payload Dados adicionais associados ao comando.
 * @param length Tamanho do payload em bytes.
 * @return SUBSYSTEM_OK se o comando for enviado com sucesso.
 */
SubsystemStatus_t Payload_HandleCommand(uint8_t command_id, const uint8_t *payload, uint16_t length);

/**
 * @brief Executa a verificação de saúde da comunicação UART com o Raspberry Pi.
 * @return true se a resposta (ping/heartbeat) estiver dentro do tempo limite.
 */
bool Payload_HealthCheck(void);

// --- Funções Específicas de Comunicação UART ---

/**
 * @brief Envia um pacote de bytes bruto encapsulado para a RPi via UART.
 * @param data Ponteiro para os dados a serem transmitidos.
 * @param len Tamanho dos dados em bytes.
 */
void Payload_SendData(const uint8_t* data, size_t len);

/**
 * @brief Verifica se há comunicação ativa válida no enlace UART.
 * @return true se a RPi estiver respondendo dentro do tempo limite (heartbeat).
 */
bool Payload_ReadResponse(void);

#endif // PAYLOAD_H
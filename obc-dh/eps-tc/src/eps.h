#ifndef EPS_H
#define EPS_H

#include <Arduino.h>
#include "../../common/interfaces/subsystem_types.h"

// Estrutura de Telemetria do EPS
typedef struct {
    float bus_voltage; // Tensão do barramento (V)
    float crnt_mA;     // Corrente da bateria (mA)
    float pwr_mW;      // Potência consumida (mW)
    float temp_C;      // Temperatura da Placa OBC (°C)
} EPS_Telemetry_t;

// --- Interface Padrão do Módulo EPS ---

/**
 * @brief Inicializa o hardware do EPS (sensores INA219, barramento I2C, etc).
 * @return SUBSYSTEM_OK se a inicialização for bem-sucedida.
 */
SubsystemStatus_t EPS_Init(void);

/**
 * @brief Tarefa principal do EPS executada pelo FreeRTOS.
 * @param pvParameters Parâmetros de inicialização da task.
 */
void EPS_Task(void *pvParameters);

/**
 * @brief Obtém os dados de telemetria atualizados do EPS.
 * @param out_telemetry Ponteiro para a estrutura onde a telemetria será gravada.
 * @return SUBSYSTEM_OK se as leituras forem válidas.
 */
SubsystemStatus_t EPS_GetTelemetry(EPS_Telemetry_t *out_telemetry);

/**
 * @brief Trata comandos recebidos via TC (Telecomanda) destinados ao EPS.
 * @param command_id Identificador do comando.
 * @param payload Dados adicionais associados ao comando.
 * @param length Tamanho do payload em bytes.
 * @return SUBSYSTEM_OK se o comando for executado com sucesso.
 */
SubsystemStatus_t EPS_HandleCommand(uint8_t command_id, const uint8_t *payload, uint16_t length);

/**
 * @brief Realiza a verificação de saúde e limites de segurança das baterias/tensão.
 * @return true se o sistema estiver operando em parâmetros seguros, false caso contrário.
 */
bool EPS_HealthCheck(void);

#endif // EPS_H
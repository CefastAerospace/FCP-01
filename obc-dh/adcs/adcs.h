#ifndef ADCS_H
#define ADCS_H

#include <Arduino.h>

// Enumeração de status padrão para o subsistema
typedef enum {
    SUBSYSTEM_OK = 0,
    SUBSYSTEM_ERR_INIT_FAILED,
    SUBSYSTEM_ERR_COMMS,
    SUBSYSTEM_ERR_INVALID_PARAM,
    SUBSYSTEM_ERR_TIMEOUT
} SubsystemStatus_t;

// Modos de operação do ADCS
typedef enum {
    ADCS_MODE_IDLE = 0,
    ADCS_MODE_ACTIVE,
    ADCS_MODE_EMERGENCY_STOP
} ADCS_Mode_t;

// Estrutura de dados do sensor IMU (Aceleração e Velocidade Angular)
typedef struct {
    float ax, ay, az; // Aceleração (m/s²)
    float gx, gy, gz; // Velocidade Angular (rad/s ou °/s)
} IMUData_t;

// Estrutura completa de telemetria do ADCS
typedef struct {
    IMUData_t imu;          // Dados dos sensores de orientação
    float current_rpm;      // Velocidade atual da roda de reação (RPM)
    float target_rpm;       // Velocidade alvo da roda de reação (RPM)
    ADCS_Mode_t mode;       // Modo operacional atual
} ADCS_Telemetry_t;

// --- Interface Padrão do Módulo ADCS ---

/**
 * @brief Inicializa o IMU (MPU6050) e os pinos do driver do motor (SimpleFOC).
 * @return SUBSYSTEM_OK se a inicialização do hardware for bem-sucedida.
 */
SubsystemStatus_t ADCS_Init(void);

/**
 * @brief Tarefa principal do ADCS executada pelo FreeRTOS no Core 1.
 * @param pvParameters Parâmetros de inicialização da tarefa.
 */
void ADCS_Task(void *pvParameters);

/**
 * @brief Obtém os dados de telemetria atualizados (IMU + Estado do Motor).
 * @param out_telemetry Ponteiro para a estrutura onde a telemetria será gravada.
 * @return SUBSYSTEM_OK se as leituras forem válidas.
 */
SubsystemStatus_t ADCS_GetTelemetry(ADCS_Telemetry_t *out_telemetry);

/**
 * @brief Processa e executa comandos de telecomanda (TC) direcionados ao ADCS.
 * @param command_id Identificador do comando.
 * @param payload Dados adicionais associados ao comando.
 * @param length Tamanho do payload em bytes.
 * @return SUBSYSTEM_OK se o comando for executado com sucesso.
 */
SubsystemStatus_t ADCS_HandleCommand(uint8_t command_id, const uint8_t *payload, uint16_t length);

/**
 * @brief Realiza a verificação de saúde do IMU e limites operacionais do motor.
 * @return true se os parâmetros estiverem dentro das margens seguras, false caso contrário.
 */
bool ADCS_HealthCheck(void);

// --- Funções Específicas do Módulo ---

/**
 * @brief Realiza a leitura direta dos dados brutos/filtrados do IMU.
 * @return Estrutura IMUData_t com os eixos de aceleração e giro.
 */
IMUData_t ADCS_ReadIMU(void);

/**
 * @brief Define a velocidade de rotação da roda de reação.
 * @param targetRPM Velocidade alvo em RPM.
 */
void ADCS_SetMotorSpeed(float targetRPM);

/**
 * @brief Corta o acionamento do motor imediatamente em situação de falha ou comando crítico.
 */
void ADCS_EmergencyStop(void);

#endif // ADCS_H
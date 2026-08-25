#ifndef SUBSYSTEM_TYPES_H
#define SUBSYSTEM_TYPES_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Códigos de status e erros padronizados para todos os subsistemas do CubeSat.
 */
typedef enum {
    SUBSYSTEM_OK = 0,               /*!< Operação realizada com sucesso */
    SUBSYSTEM_ERR_INIT_FAILED,      /*!< Falha na inicialização do hardware/periférico */
    SUBSYSTEM_ERR_COMMS,            /*!< Falha de comunicação no barramento (I2C, SPI, UART) */
    SUBSYSTEM_ERR_INVALID_PARAM,    /*!< Parâmetro ou comando inválido recebido */
    SUBSYSTEM_ERR_TIMEOUT,          /*!< Tempo limite de resposta excedido */
    SUBSYSTEM_ERR_NOT_READY,        /*!< Subsistema ainda não inicializado ou ocupado */
    SUBSYSTEM_ERR_HARDWARE_FAULT    /*!< Falha física ou leitura fora da faixa operacional */
} SubsystemStatus_t;

/**
 * @brief Identificadores dos subsistemas do FCP-01.
 */
typedef enum {
    SUBSYSTEM_ID_OBC = 0,
    SUBSYSTEM_ID_EPS,
    SUBSYSTEM_ID_TTC,
    SUBSYSTEM_ID_ADCS,
    SUBSYSTEM_ID_PAYLOAD,
    SUBSYSTEM_ID_RTC,
    SUBSYSTEM_ID_SD
} SubsystemID_t;

/**
 * @brief Modos operacionais gerais da missão do CubeSat.
 */
typedef enum {
    SAT_MODE_BOOT = 0,
    SAT_MODE_SAFE,
    SAT_MODE_NOMINAL,
    SAT_MODE_PAYLOAD,
    SAT_MODE_CRITICAL
} SatMode_t;

#endif // SUBSYSTEM_TYPES_H
#ifndef SUBSYSTEM_TYPES_H
#define SUBSYSTEM_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include "../../src/subsystem_interface.h"

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
#ifndef BASE_COMMS_H
#define BASE_COMMS_H
#include <Arduino.h>
#include "sd_logger.h"

// UART - RPI ZERO W
#define PIN_RXpld 16
#define PIN_TXpld 17

#define BASE_BAUDRATE 115200
#define SERIAL_BUF_SIZE 1024

// QUEUE
#define BASE_QUEUE_LENGTH 20

// INICIALIZAÇÃO
bool Base_Init();
bool Base_StartTask();

// ENVIO
bool Base_SendPacket(const LogPacket& packet);

#endif

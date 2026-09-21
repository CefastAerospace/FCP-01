#ifndef PAYLOAD_PROTOCOL_H
#define PAYLOAD_PROTOCOL_H

#include <stdint.h>

// --- Delimitadores do protocolo UART (ESP32 <-> RPi Zero W) ---
#define UART_START_BYTE 0xAA
#define UART_END_BYTE   0x55

// --- Tipos de pacote ---
#define UART_PKT_PING           0x00
#define UART_PKT_CMD            0x01
#define UART_PKT_PAYLOAD_DATA   0x10

// --- Comandos ESP32 -> RPi ---
#define PAYLOAD_CMD_START_ACQ   0x10
#define PAYLOAD_CMD_STOP_ACQ    0x11
#define PAYLOAD_CMD_SHUTDOWN    0x1F

// Cabeçalho de quadro UART
typedef struct __attribute__((packed)) {
    uint8_t  start_byte;
    uint8_t  message_id;
    uint8_t  packet_type;
    uint8_t  payload_len;
} UartFrameHeader_t;

// Pacote de dados de telemetria da RPi (payload do UART_PKT_PAYLOAD_DATA)
typedef struct __attribute__((packed)) {
    uint32_t timestamp;
    uint32_t adsb_messages_count;
    float    pi_cpu_temp;
    uint8_t  predictor_state;
} PayloadDataPackage_t;

#endif // PAYLOAD_PROTOCOL_H

#include "payload.h"
#include "../../common/interfaces/payload_protocol.h"
#include "sd_logger.h"

// Instância global de telemetria interna do subsistema
static Payload_Telemetry_t g_payload_telemetry = {
    .adsb_messages_received = 0,
    .last_sync_timestamp = 0,
    .is_pi_responsive = false,
    .pi_temperature_c = 0.0f
};

static uint8_t g_msg_sequence = 0;
static uint32_t g_last_ping_time = 0;

// Utilizando Serial2 do ESP32 para comunicação com a RPi Zero W
#define PAYLOAD_UART Serial2
#define UART_RX_PIN 16
#define UART_TX_PIN 17

// --- Função Auxiliar de Cálculo de CRC16 ---
static uint16_t UART_CalculateCRC16(const uint8_t *data, size_t len) {
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (uint8_t j = 0; j < 8; j++) {
            if (crc & 0x0001) crc = (crc >> 1) ^ 0xA001;
            else crc >>= 1;
        }
    }
    return crc;
}

// --- Interface Padrão ---

SubsystemStatus_t Payload_Init(void) {
    // Inicializa a UART2 a 115200 baud nos pinos GPIO 16 (RX) e 17 (TX)
    PAYLOAD_UART.begin(115200, SERIAL_8N1, UART_RX_PIN, UART_TX_PIN);
    g_last_ping_time = millis();
    return SUBSYSTEM_OK;
}

void Payload_Task(void *pvParameters) {
    uint8_t rx_buffer[256];

    for (;;) {
        // Verifica se há quadros UART completos disponíveis
        if (PAYLOAD_UART.available() > 0) {
            // Sincronização por START_BYTE (0xAA)
            if (PAYLOAD_UART.read() == UART_START_BYTE) {
                UartFrameHeader_t header;
                header.start_byte = UART_START_BYTE;
                
                // Lê o restante do cabeçalho
                PAYLOAD_UART.readBytes((uint8_t*)&header.message_id, 3);

                // Garante leitura de todo o payload + CRC + END_BYTE
                size_t bytes_to_read = header.payload_len + sizeof(uint16_t) + 1;
                if (PAYLOAD_UART.readBytes(rx_buffer, bytes_to_read) == bytes_to_read) {
                    
                    uint16_t received_crc = *(uint16_t*)&rx_buffer[header.payload_len];
                    uint8_t end_byte = rx_buffer[header.payload_len + sizeof(uint16_t)];

                    // Valida delimitador final (0x55) e integridade CRC16
                    if (end_byte == UART_END_BYTE) {
                        uint16_t calculated_crc = UART_CalculateCRC16((uint8_t*)&header, sizeof(header));
                        calculated_crc = UART_CalculateCRC16(rx_buffer, header.payload_len); // acumula payload

                        // Processamento do pacote válido
                        g_last_ping_time = millis();
                        g_payload_telemetry.is_pi_responsive = true;

                        switch (header.packet_type) {
                            case UART_PKT_PING:
                                // Heartbeat recebido da RPi Zero W
                                break;

                            case UART_PKT_PAYLOAD_DATA: {
                                PayloadDataPackage_t *data = (PayloadDataPackage_t*)rx_buffer;
                                g_payload_telemetry.adsb_messages_received = data->adsb_messages_count;
                                g_payload_telemetry.last_sync_timestamp = data->timestamp;
                                g_payload_telemetry.pi_temperature_c = data->pi_cpu_temp;
                                break;
                            }

                            default:
                                break;
                        }
                    }
                }
            }
        }

        // Checagem de Timeout da RPi (Heartbeat > 6 segundos = Inoperante)
        if (millis() - g_last_ping_time > 6000) {
            g_payload_telemetry.is_pi_responsive = false;
        }

        vTaskDelay(pdMS_TO_TICKS(50)); // Task roda a 20Hz
    }
}

SubsystemStatus_t Payload_GetTelemetry(Payload_Telemetry_t *out_telemetry) {
    if (out_telemetry == NULL) return SUBSYSTEM_ERR_INVALID_PARAM;
    *out_telemetry = g_payload_telemetry;
    return SUBSYSTEM_OK;
}

SubsystemStatus_t Payload_HandleCommand(uint8_t command_id, const uint8_t *payload, uint16_t length) {
    // Monta o frame UART para enviar comandos do ESP32 -> RPi Zero W
    uint8_t tx_buffer[262];
    UartFrameHeader_t header = {
        .start_byte = UART_START_BYTE,
        .message_id = g_msg_sequence++,
        .packet_type = UART_PKT_CMD,
        .payload_len = (uint8_t)(length + 1) // command_id + dados
    };

    memcpy(tx_buffer, &header, sizeof(header));
    tx_buffer[sizeof(header)] = command_id;
    if (length > 0 && payload != NULL) {
        memcpy(&tx_buffer[sizeof(header) + 1], payload, length);
    }

    uint16_t crc = UART_CalculateCRC16(tx_buffer, sizeof(header) + header.payload_len);
    size_t offset = sizeof(header) + header.payload_len;

    memcpy(&tx_buffer[offset], &crc, sizeof(crc));
    tx_buffer[offset + sizeof(crc)] = UART_END_BYTE;

    PAYLOAD_UART.write(tx_buffer, offset + sizeof(crc) + 1);
    return SUBSYSTEM_OK;
}

bool Payload_HealthCheck(void) {
    return g_payload_telemetry.is_pi_responsive;
}

void Payload_SendData(const char* data) {
    if (data != NULL) {
        Payload_HandleCommand(PAYLOAD_CMD_START_ACQ, (const uint8_t*)data, strlen(data));
    }
}

bool Payload_ReadResponse(char* buffer, size_t maxLen) {
    return g_payload_telemetry.is_pi_responsive;
}
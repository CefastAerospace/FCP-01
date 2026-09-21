#ifndef TTC_H
#define TTC_H

#include <Arduino.h>
#include "../../src/subsystem_interface.h"
#include "../../src/conops.h"
#include "../../src/drivers/lora_sx1276.h"
#ifdef TTC_USE_MOCK
#include "../../src/drivers/lora_mock.h"
#endif
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

// Status de resposta dos Telecomandos (ACK/NACK)
typedef enum : uint8_t {
    ACK_OK               = 0x00,
    ERR_CRC              = 0x01,
    ERR_INVALID_CMD      = 0x02,
    ERR_INVALID_PARAM    = 0x03,
    ERR_STATE_REJECTED   = 0x04,
    ERR_UNKNOWN_CMD      = 0xFF
} CmdStatus_t;

// Comandos internos OBC -> TTC (via SubsystemCommand_t / HandleCommand)
typedef enum : uint8_t {
    TTC_CMD_TX_TELEMETRY   = 0x01,  // Transmite um pacote de telemetria agora
    TTC_CMD_BEACON_ON      = 0x02,  // Habilita beacon periódico
    TTC_CMD_BEACON_OFF     = 0x03,  // Desabilita beacon
    TTC_CMD_SET_BEACON_INT = 0x04,  // Define intervalo de beacon (ms, uint32 no payload)
    TTC_CMD_TX_RAW         = 0x05,  // Transmite payload bruto (bytes no cmd.payload)
    TTC_CMD_RESET_STATS    = 0x06   // Zera contadores de link
} TTC_InternalCmd_t;

// Opcodes de Entrada
typedef enum : uint8_t {
    CMD_PING           = 0x01,
    CMD_SET_MODE       = 0x02,
    CMD_START_MISSION  = 0x03,
    CMD_STOP_MISSION   = 0x04,
    CMD_GET_STATUS     = 0x05,
    CMD_ENTER_SAFE     = 0x0A,
    CMD_EXIT_SAFE      = 0x0B,
    CMD_REQUEST_TELEMETRY = 0x0C,
    CMD_SET_TIME       = 0x10,
    CMD_ADCS_START     = 0x20,
    CMD_ADCS_STOP      = 0x21,
    CMD_DEPLOY_ANTENNA = 0x30,
    CMD_REQUEST_LOG    = 0x40,
    CMD_RESET_OBC      = 0xFF
} CommandID_t;

// Estruturas de pacotes originais mantidas
typedef struct __attribute__((packed)) {
    uint16_t sequence_id;
    uint8_t  command_id;
    uint8_t  flags;
    uint32_t timestamp;
    uint8_t  arguments[8];
    uint16_t checksum;
} TelecommandPacket_t;

typedef struct __attribute__((packed)) {
    uint16_t header;
    uint16_t sequence_id;
    uint8_t  command_id;
    uint8_t  status_code;
    uint16_t checksum;
} ACKPacket_t;

// Pacote de Telemetria (Downlink) — espelha o padrão do ACK com frame + checksum
#define TM_HEADER        0xAA55
#define TM_TYPE_TELEMETRY 0x01

typedef struct __attribute__((packed)) {
    uint16_t header;           // 0xAA55
    uint16_t sequence_id;      // Contador incremental de pacotes TM
    uint8_t  packet_type;      // TM_TYPE_TELEMETRY
    uint8_t  system_status;    // Estado ConOps (SystemState_t)
    uint32_t timestamp;        // Tempo do OBC (millis)
    // --- EPS ---
    float vbat;                // Tensão do barramento (V)
    float ibat;                // Corrente da bateria (mA)
    float temp_obc;            // Temperatura OBC (°C)
    // --- ADCS ---
    float rpm;                 // Roda de reação atual (RPM)
    float target_rpm;          // Roda de reação alvo (RPM)
    // --- Link TT&C ---
    int16_t  last_rssi;        // RSSI do último pacote RX (dBm)
    float    last_snr;         // SNR do último pacote RX (dB)
    uint32_t rx_packets_count; // Pacotes RX válidos
    uint32_t tx_packets_count; // Pacotes TX enfileirados
    uint32_t rx_errors_count;  // Erros RX (ex: CRC inválido)
    uint16_t checksum;         // CRC16 sobre os campos anteriores
} TelemetryPacket_t;

typedef struct {
    int16_t  last_rssi;
    float    last_snr;
    uint32_t rx_packets_count;
    uint32_t tx_packets_count;
    uint32_t rx_errors_count;
    bool     is_transmitting;
} TTC_Telemetry_t;

// Classe que implementa a interface padrão
class TTC_Module : public ISubsystem {
private:
    TTC_Telemetry_t _telemetry;
    ILoRaRadio* _radio = NULL;
    QueueHandle_t _txQueue = NULL;
    uint16_t _sequence_id = 0;
    int64_t _time_offset_ms = 0;
    bool     _beacon_enabled = false;
    uint32_t _beacon_interval_ms = 10000;
    uint32_t _last_beacon_ms = 0;
    bool     _burn_active = false;
    uint32_t _burn_end_ms = 0;

public:
    TTC_Module();

    // --- Métodos do Contrato ISubsystem ---
    SubsystemStatus_t Init() override;
    void TaskUpdate() override;
    SubsystemStatus_t GetTelemetry(uint8_t *buffer, size_t max_len, size_t *out_len) override;
    SubsystemStatus_t HandleCommand(const SubsystemCommand_t &cmd) override;
    bool HealthCheck() override;
    const char* GetName() override { return "TTC"; }

    // --- Funções Específicas do Rádio ---
    uint16_t CalculateCRC16(const uint8_t *data, size_t len);
    void SendACK(uint16_t sequence_id, uint8_t command_id, uint8_t status_code);
    void ProcessPacket(const TelecommandPacket_t &pkt);
    void SendTelemetryPacket();
    void FlushTxQueue();
    bool QueueTxPacket(const uint8_t* data, size_t len);
};

extern TTC_Module TTC;

#ifdef TTC_USE_MOCK
// Acesso ao mock para autoteste (só existe no env esp32dev-mock)
LoRaMock* TTC_GetMock();
#endif

#endif // TTC_H

#ifndef TTC_H
#define TTC_H

#include <Arduino.h>
#include "subsystem_interface.h"
#include "conops.h"

// Status de resposta dos Telecomandos (ACK/NACK)
typedef enum : uint8_t {
    ACK_OK               = 0x00,
    ERR_CRC              = 0x01,
    ERR_INVALID_CMD      = 0x02,
    ERR_INVALID_PARAM    = 0x03,
    ERR_STATE_REJECTED   = 0x04,
    ERR_UNKNOWN_CMD      = 0xFF
} CmdStatus_t;

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
typedef struct {
    uint32_t timestamp;
    float voltage;
    float current;
    float temp;
    uint8_t systemStatus;
} PacketTelemetry_t;

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
};

extern TTC_Module TTC;

#endif // TTC_H

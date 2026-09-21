#ifndef LORA_MOCK_H
#define LORA_MOCK_H

#include <Arduino.h>
#include "lora_sx1276.h"

// Mock do rádio LoRa para testes sem hardware (loopback em RAM).
// Não acessa SPI nem o SX1276 físico.
class LoRaMock : public ILoRaRadio {
private:
    bool _ready = false;
    int16_t _rssi = -60;
    float _snr = 8.0f;

    uint8_t _rx_buffer[LORA_MAX_PACKET_SIZE];   // Mensagem "recebida" injetada pelo teste
    size_t  _rx_len = 0;
    bool    _rx_pending = false;

    uint8_t _tx_buffer[LORA_MAX_PACKET_SIZE];   // Última mensagem "transmitida"
    size_t  _tx_len = 0;

public:
    bool Init() override;                       // Apenas marca pronto
    bool Send(const uint8_t* data, size_t len) override;             // "Transmite": copia para _tx_buffer
    int  Receive(uint8_t* buffer, size_t max_len) override;          // Entrega _rx_buffer uma vez
    int16_t GetRSSI() override { return _rssi; }
    float   GetSNR()  override { return _snr; }
    bool IsAvailable() override { return _ready; }
    void Sleep() override {}
    void Idle() override {}
    void ReceiveMode() override {}

    // --- Funções auxiliares de teste ---
    void InjectPacket(const uint8_t* data, size_t len);   // Simula recepção de um pacote
    void SetLinkQuality(int16_t rssi, float snr);
    const uint8_t* GetLastTx(size_t* out_len) const;      // Inspeciona o último pacote TX
    uint32_t tx_count = 0;
};

#endif // LORA_MOCK_H

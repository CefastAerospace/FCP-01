#include "lora_mock.h"

bool LoRaMock::Init() {
    _ready = true;
    _rx_len = 0;
    _rx_pending = false;
    _tx_len = 0;
    return true;
}

bool LoRaMock::Send(const uint8_t* data, size_t len) {
    if (data == NULL || len == 0 || len > LORA_MAX_PACKET_SIZE) return false;
    memcpy(_tx_buffer, data, len);
    _tx_len = len;
    tx_count++;
    return true;
}

int LoRaMock::Receive(uint8_t* buffer, size_t max_len) {
    if (buffer == NULL || max_len == 0) return 0;
    if (!_rx_pending) return 0;

    size_t to_copy = (_rx_len < max_len) ? _rx_len : max_len;
    memcpy(buffer, _rx_buffer, to_copy);
    _rx_pending = false;   // Entrega uma única vez até novo InjectPacket
    return (int)to_copy;
}

void LoRaMock::InjectPacket(const uint8_t* data, size_t len) {
    if (data == NULL || len > LORA_MAX_PACKET_SIZE) return;
    memcpy(_rx_buffer, data, len);
    _rx_len = len;
    _rx_pending = true;
}

void LoRaMock::SetLinkQuality(int16_t rssi, float snr) {
    _rssi = rssi;
    _snr = snr;
}

const uint8_t* LoRaMock::GetLastTx(size_t* out_len) const {
    if (out_len != NULL) *out_len = _tx_len;
    return _tx_buffer;
}

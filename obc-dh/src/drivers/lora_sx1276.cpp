#include "lora_sx1276.h"
#include <SPI.h>
#include <LoRa.h>

bool LoRaDriver::Init() {
    Serial.println("[LoRa] Inicializando SX1276...");

    SPI.begin(LORA_SCK_PIN, LORA_MISO_PIN, LORA_MOSI_PIN, LORA_CS_PIN);
    LoRa.setPins(LORA_CS_PIN, LORA_RST_PIN, LORA_DIO0_PIN);

    if (!LoRa.begin(LORA_FREQ)) {
        Serial.println("[LoRa] ERRO: falha ao inicializar modulo!");
        _ready = false;
        return false;
    }

    LoRa.setSpreadingFactor(LORA_SF);
    LoRa.setSignalBandwidth(LORA_BW);
    LoRa.setCodingRate4(LORA_CR);
    LoRa.setTxPower(LORA_TXPOWER);
    LoRa.setSyncWord(LORA_SYNC_WORD);

    LoRa.receive();
    _ready = true;

    Serial.println("[LoRa] Modulo inicializado com sucesso.");
    return true;
}

bool LoRaDriver::Send(const uint8_t* data, size_t len) {
    if (data == NULL || len == 0 || len > LORA_MAX_PACKET_SIZE) return false;

    LoRa.idle();
    LoRa.beginPacket();
    LoRa.write(data, len);
    LoRa.endPacket();
    LoRa.receive();

    return true;
}

int LoRaDriver::Receive(uint8_t* buffer, size_t max_len) {
    if (buffer == NULL || max_len == 0) return 0;

    int packetSize = LoRa.parsePacket();
    if (packetSize == 0) return 0;

    int read = 0;
    while (LoRa.available() && read < (int)max_len) {
        buffer[read++] = LoRa.read();
    }
    return read;
}

int16_t LoRaDriver::GetRSSI() {
    return LoRa.packetRssi();
}

float LoRaDriver::GetSNR() {
    return LoRa.packetSnr();
}

bool LoRaDriver::IsAvailable() {
    return _ready;
}

void LoRaDriver::Sleep() {
    LoRa.sleep();
}

void LoRaDriver::Idle() {
    LoRa.idle();
}

void LoRaDriver::ReceiveMode() {
    LoRa.receive();
}

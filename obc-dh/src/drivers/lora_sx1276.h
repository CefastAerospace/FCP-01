#ifndef LORA_SX1276_H
#define LORA_SX1276_H

#include <Arduino.h>

// --- Pinagem VSPI - LoRa SX1276 ---
#define LORA_SCK_PIN   18
#define LORA_MISO_PIN  19
#define LORA_MOSI_PIN  23
#define LORA_CS_PIN    5
#define LORA_RST_PIN   14
#define LORA_DIO0_PIN  4

// --- Parâmetros de Rádio ---
#define LORA_FREQ       915E6
#define LORA_SF         9
#define LORA_BW         125E3
#define LORA_CR         6
#define LORA_TXPOWER    17
#define LORA_SYNC_WORD  0x12

// --- Limites ---
#define LORA_MAX_PACKET_SIZE 256

typedef struct {
    uint8_t data[LORA_MAX_PACKET_SIZE];
    uint8_t len;
} LoRaPacket_t;

// Interface abstrata do rádio — permite alternar driver real (SX1276) e mock (testes)
class ILoRaRadio {
public:
    virtual ~ILoRaRadio() {}
    virtual bool Init() = 0;
    virtual bool Send(const uint8_t* data, size_t len) = 0;
    virtual int  Receive(uint8_t* buffer, size_t max_len) = 0;
    virtual int16_t GetRSSI() { return 0; }
    virtual float   GetSNR()  { return 0.0f; }
    virtual bool IsAvailable() = 0;
    virtual void Sleep() {}
    virtual void Idle() {}
    virtual void ReceiveMode() {}
};

// --- Driver real: SX1276 via SPI (sandeepmistry/LoRa) ---
class LoRaDriver : public ILoRaRadio {
private:
    bool _ready = false;

public:
    bool Init() override;
    bool Send(const uint8_t* data, size_t len) override;
    int  Receive(uint8_t* buffer, size_t max_len) override;
    int16_t GetRSSI() override;
    float   GetSNR() override;
    bool IsAvailable() override;
    void Sleep() override;
    void Idle() override;
    void ReceiveMode() override;
};

#endif // LORA_SX1276_H

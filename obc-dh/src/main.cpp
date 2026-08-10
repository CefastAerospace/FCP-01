#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <HardwareSerial.h>
#include "../eps-tc/src/eps.h"

// Pinagem
#define LED_BUILTIN 2
// UART - RPi zero W
#define PIN_RXpld 16
#define PIN_TXpld 17

// I2C - TC, EPS e ADCS
#define PIN_SDAitc 21
#define PIN_SCLitc 22

// VSPI - LoRa SX1276 TT&C 
#define PIN_SCKv 18
#define PIN_MISOv 19
#define PIN_MOSIv 23
#define PIN_CSv 5
#define PIN_RSTv 14
#define PIN_DIO0v 4

// HSPI - SD card OBC
#define PIN_SCKh 15
#define PIN_MISOh 35 // Input-only
#define PIN_MOSIh 12
#define PIN_CSh 13

// PWM - SimpleFOC
#define PIN_PWMu 25
#define PIN_PWMv 26
#define PIN_PWMw 27
#define PIN_EN 32

// Burn wire - Gatilho
#define PIN_BURN 33

// INSTANCIAS
HardwareSerial SerialPLD(2); // UART - RPi zero W
SPIClass SPIh(HSPI); // HSPI - SD card

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  Serial.begin(115200);

  unsigned long startWait = millis();
  while (!Serial && (millis() - startWait < 3000))
  {
    delay(100);
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN)); // Pisca o LED enquanto espera a inicialização da Serial
  }
  
  digitalWrite(LED_BUILTIN, HIGH);
  Serial.println("\nOBC FCP-01 Inicializando");
  
  if (EPSinit()) {
    Serial.println("[OK] EPS inicializado com sucesso");
  } else {
    Serial.println("[ERRO] Falha na inicialização do EPS");
  }

  pinMode(PIN_BURN, OUTPUT);
  digitalWrite(PIN_BURN, LOW); // Gatilho do Burn Wire desligado

  pinMode(PIN_EN, OUTPUT);
  digitalWrite(PIN_EN, LOW); // Driver do motor desligado

  // Inicializa UART2
    SerialPLD.begin(115200, SERIAL_8N1, PIN_RXpld, PIN_TXpld); // Inicializa UART2 com baudrate de 115200, 8 bits de dados, sem paridade e 1 bit de parada
    Wire.begin(PIN_SDAitc, PIN_SCLitc, 400000); // Inicializa I2C com frequência de 400kHz
    SPIh.begin(PIN_SCKh, PIN_MISOh, PIN_MOSIh, PIN_CSh); // Inicializa HSPI com pinos definidos
    SPI.begin(PIN_SCKv, PIN_MISOv, PIN_MOSIv, PIN_CSv); // Inicializa VSPI com pinos definidos
    Serial.println("Interfaces de comunicação inicializadas");


}
void loop() {
  // Coloque aqui o código principal do seu programa
}
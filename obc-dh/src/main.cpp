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

// Cada bit representa um subsistema que terminou sua inicialização.
#define EPS_READY       (1 << 0)
#define UART_READY      (1 << 1)
#define I2C_READY       (1 << 2)
#define HSPI_READY      (1 << 3)
#define VSPI_READY      (1 << 4)
#define INIT_ERROR      (1 << 5)
#define MISSION_READY   (1 << 6)

// Event Group
EventGroupHandle_t systemEvents;

// TASK - INICIALIZAÇÃO DO EPS
void TaskEPS(void *parameter) { Serial.println("[BOOT] Inicializando EPS...");
  if (EPSinit()) {
    Serial.println("[OK] EPS inicializado");
    xEventGroupSetBits(systemEvents, EPS_READY);} 
  else {
    Serial.println("[ERRO] Falha na inicialização do EPS");
    xEventGroupSetBits(systemEvents, INIT_ERROR);}
  vTaskDelete(NULL);}

// TASK - INICIALIZAÇÃO DAS COMUNICAÇÕES
void TaskCommunication(void *parameter) {
  Serial.println("[BOOT] Inicializando interfaces de comunicação...");
 SerialPLD.begin( 115200, SERIAL_8N1, PIN_RXpld, PIN_TXpld);
  Serial.println("[OK] UART inicializada");
  xEventGroupSetBits(systemEvents, UART_READY);

// I2C - TC, EPS e ADCS
Wire.begin(PIN_SDAitc, PIN_SCLitc, 400000);
  Serial.println("[OK] I2C inicializado");
  xEventGroupSetBits(systemEvents, I2C_READY);
 vTaskDelete(NULL);}

// TASK - INICIALIZAÇÃO DAS INTERFACES SPI
void TaskSPI(void *parameter) {
  Serial.println("[BOOT] Inicializando interfaces SPI...");

 // HSPI - SD Card
SPIh.begin(PIN_SCKh, PIN_MISOh, PIN_MOSIh, PIN_CSh);
  Serial.println("[OK] HSPI (SD Card) inicializado");
  xEventGroupSetBits(systemEvents, HSPI_READY);

  // VSPI - LoRa SX1276
 SPI.begin(PIN_SCKv, PIN_MISOv, PIN_MOSIv, PIN_CSv);
  Serial.println("[OK] VSPI (LoRa SX1276) inicializado");
  xEventGroupSetBits(systemEvents, VSPI_READY);
 vTaskDelete(NULL);}

// TASK - GERENCIADOR DE BOOT
void TaskBootManager(void *parameter) {
  Serial.println("[BOOT] Aguardando inicialização dos subsistemas...");
const EventBits_t requiredBits =
    EPS_READY |
    UART_READY |
    I2C_READY |
    HSPI_READY |
    VSPI_READY;
  while (1) {
EventBits_t bits = xEventGroupWaitBits(systemEvents, requiredBits | INIT_ERROR, pdFALSE, pdFALSE, portMAX_DELAY);

  // VERIFICA SE OCORREU ERRO
if (bits & INIT_ERROR) {
      Serial.println();
      Serial.println();
      Serial.println("   FALHA NA INICIALIZAÇÃO");
      Serial.println();
      Serial.println();

 // LED piscando indica falha crítica
      while (1) {
        digitalWrite( LED_BUILTIN, !digitalRead(LED_BUILTIN));
vTaskDelay(pdMS_TO_TICKS(500));
      }
    }

 // VERIFICA SE TODOS OS SUBSISTEMAS ESTÃO PRONTOS
if ((bits & requiredBits) == requiredBits) {
      Serial.println();
      Serial.println();
      Serial.println("          OBC FCP-01");
      Serial.println("      PRONTO PARA MISSÃO");
      Serial.println();
      Serial.println();
// Marca o sistema como pronto para missão
      xEventGroupSetBits(systemEvents, MISSION_READY);
      break;}
  }
  // Boot concluído
  vTaskDelete(NULL);}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW); // Led de inicialização do ESP
  pinMode(PIN_BURN, OUTPUT);
  digitalWrite(PIN_BURN, LOW); // Gatilho do Burn Wire desligado
  pinMode(PIN_EN, OUTPUT);
  digitalWrite(PIN_EN, LOW); // Driver do motor desligado
  Serial.begin(115200);
  unsigned long startWait = millis();
  while (!Serial && (millis() - startWait < 3000))
  {delay(100);
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));} // Pisca o LED enquanto espera a inicialização da Serial
  digitalWrite(LED_BUILTIN, HIGH);
   Serial.println("       OBC FCP-01 BOOT");
  systemEvents = xEventGroupCreate();
  // Falha crítica
  if (systemEvents == NULL) {
    Serial.println(
      "[FATAL] Falha ao criar Event Group!");
    while (1) {
      digitalWrite(
        LED_BUILTIN,
        !digitalRead(LED_BUILTIN));
      delay(200);}
  }
  Serial.println("[BOOT] Event Group criado");

  xTaskCreate(TaskEPS, "TaskEPS", 4096, NULL, 2, NULL);
  xTaskCreate(TaskCommunication, "TaskCommunication", 4096, NULL, 2, NULL);
  xTaskCreate(TaskSPI, "TaskSPI", 4096, NULL, 2, NULL);
  xTaskCreate(TaskBootManager, "TaskBootManager", 4096, NULL, 3, NULL);
  Serial.println("[BOOT] Tasks de inicialização criadas");
}
void loop() {
 vTaskDelay(
    pdMS_TO_TICKS(1000)
  );
}

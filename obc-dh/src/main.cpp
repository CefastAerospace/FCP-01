#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include "../eps-tc/src/eps.h"
#include "esp_heap_caps.h"
#include "sd_logger.h"
#include "base_comms.h"
#include <RTClib.h>
#include "esp_task_wdt.h"

// Pinagem
#define LED_BUILTIN 2

//Instanciação do RTC
RTC_DS3231 rtc;

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

// PWM - SimpleFOC
#define PIN_PWMu 25
#define PIN_PWMv 26
#define PIN_PWMw 27
#define PIN_EN 32

// Burn wire - Gatilho
#define PIN_BURN 33

// Cada bit representa um subsistema que terminou sua inicialização.
#define EPS_READY       (1 << 0)
#define UART_READY      (1 << 1)
#define I2C_READY       (1 << 2)
#define HSPI_READY      (1 << 3)
#define RTC_READY       (1 << 4)
#define VSPI_READY      (1 << 5)
#define INIT_ERROR      (1 << 6)
#define MISSION_READY   (1 << 7)

// Event Group
EventGroupHandle_t systemEvents;

//Timestamp universal
void GetCurrentTimestamp(char* buffer, size_t maxLen) {
    DateTime now = rtc.now();
    snprintf(buffer, maxLen, "%04d-%02d-%02dT%02d:%02d:%02d", 
             now.year(), now.month(), now.day(), 
             now.hour(), now.minute(), now.second());
}

// TASK - INICIALIZAÇÃO DO EPS
void TaskEPS(void *parameter) { Serial.println("[BOOT] Inicializando EPS...");
 if (EPSinit()) {
    Serial.println("[OK] EPS inicializado");
    xEventGroupSetBits(systemEvents, EPS_READY);} 
  
  else {
    Serial.println("[ERRO] Falha na inicialização do EPS");
    xEventGroupSetBits(systemEvents, INIT_ERROR);}
  vTaskDelete(NULL);}

// I2C - TC, EPS e ADCS
void TaskCommunication(void *parameter) {
  if (Wire.begin(PIN_SDAitc, PIN_SCLitc, 400000)) {
    if (rtc.begin()) {
      if (rtc.lostPower()) {
        // Se a bateria do RTC descarregou, usa a data/hora da compilação como fallback
        rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
      }
      xEventGroupSetBits(systemEvents, I2C_READY | RTC_READY);
    } else {
      xEventGroupSetBits(systemEvents, INIT_ERROR);
    }
  } else {
    xEventGroupSetBits(systemEvents, INIT_ERROR);
  }
  vTaskDelete(NULL);
}

// TASK - INICIALIZAÇÃO DAS INTERFACES SPI
void TaskSPI(void *parameter) {
  Serial.println("[BOOT] Inicializando interfaces SPI...");
  // VSPI - LoRa SX1276
 SPI.begin(PIN_SCKv, PIN_MISOv, PIN_MOSIv, PIN_CSv);
  Serial.println("[OK] VSPI (LoRa SX1276) inicializado");
  xEventGroupSetBits(systemEvents, VSPI_READY);
 vTaskDelete(NULL);}

// TASK - GERENCIADOR DE BOOT
void TaskBootManager(void *parameter) {
  const EventBits_t requiredBits = EPS_READY | I2C_READY | RTC_READY | VSPI_READY;
  EventBits_t bits = xEventGroupWaitBits(systemEvents, requiredBits | INIT_ERROR, pdFALSE, pdFALSE, portMAX_DELAY);
  if (bits & INIT_ERROR) {
    while (1) {
      digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
      vTaskDelay(pdMS_TO_TICKS(200));
    }
  }
  if ((bits & requiredBits) == requiredBits) {
    xEventGroupSetBits(systemEvents, MISSION_READY);
  }
  vTaskDelete(NULL);
}

  // VERIFICA SE OCORREU ERRO
if (bits & INIT_ERROR) {
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
      Serial.println("          OBC FCP-01");
      Serial.println("      PRONTO PARA MISSÃO");
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
//CARTÃO SD
if (SD_Init())
    { if (SD_CreateLog())
        { if (SD_StartTask())
            { Serial.println("[OBC] SD pronto.");
            } else
            { Serial.println("[OBC] ERRO: SD Task nao iniciou.");
            }
        } else
        { Serial.println("[OBC] ERRO: MISSION.CSV nao foi criado.");
        }
    }else
    { Serial.println("[OBC] ERRO: SD nao inicializado.");
    }
//BASE/RPI
if (Base_Init())
    { if (Base_StartTask())
        { Serial.println("[OBC] Comunicacao com a base pronta.");
        } else
        { Serial.println("[OBC] ERRO: Base Task nao iniciou.");
        }
    } else
    { Serial.println("[OBC] ERRO: UART da base nao inicializada.");
    }
 
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

 //Watchdog de reinicialização
 esp_task_wdt_init(10, true);
esp_task_wdt_add(NULL);
}
void loop() {
esp_task_wdt_reset();
  vTaskDelay(pdMS_TO_TICKS(1000));
}

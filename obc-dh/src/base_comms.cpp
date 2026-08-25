#include "base_comms.h"
#include <HardwareSerial.h>

static constexpr BaseType_t CORE_IO = 0;

HardwareSerial SerialPLD(2);

static QueueHandle_t xQueueBase = NULL;

static TaskHandle_t xBaseTask = NULL;

// BASE TASK
static void Base_Task(void* parameter) {
    LogPacket packet;
    Serial.println("[BASE] Base Task iniciada");
    esp_task_wdt_add(NULL);

    while (true) {
        esp_task_wdt_reset();

        if (xQueueReceive(xQueueBase, &packet, pdMS_TO_TICKS(10)) == pdTRUE) {
            SerialPLD.print("{\"data\":\""); SerialPLD.print(packet.data);
            SerialPLD.print("\",\"time\":\""); SerialPLD.print(packet.time);
            SerialPLD.print("\",\"type\":"); SerialPLD.print(packet.type);
            SerialPLD.print(",\"payload\":"); SerialPLD.print(packet.payload);
            SerialPLD.println("}");
        }

        // Loop de escuta bidirecional pronto para o parser do Payload
        while (SerialPLD.available() > 0) {
            char c = SerialPLD.read();
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
// INICIALIZAÇÃO
bool Base_Init() {
    SerialPLD.setRxBufferSize(SERIAL_BUF_SIZE); // Expande de 256 para 1024 bytes
    SerialPLD.begin(BASE_BAUDRATE, SERIAL_8N1, PIN_RXpld, PIN_TXpld);
    Serial.println("[BASE] UART inicializada.");
    return true;
}

// TASK
bool Base_StartTask()
{
    xQueueBase = xQueueCreate(
        BASE_QUEUE_LENGTH,
        sizeof(LogPacket)
    );

    if (xQueueBase == NULL)
    {
        Serial.println(
            "[BASE] ERRO ao criar Queue!"
        );

        return false;
    }

    BaseType_t result = xTaskCreatePinnedToCore(
        Base_Task,
        "Base_Task",
        4096,
        NULL,
        3,
        &xBaseTask,
        CORE_IO
    );

    if (result != pdPASS)
    {
        Serial.println(
            "[BASE] ERRO ao criar Task!"
        );

        vQueueDelete(xQueueBase);

        xQueueBase = NULL;

        return false;
    }

    Serial.println(
        "[BASE] Task criada!"
    );

    return true;
}

// ENVIO
bool Base_SendPacket(const LogPacket& packet) {
    if (xQueueBase == NULL) return false;

    if (packet.type == LOG_EVENT || packet.type == LOG_SYSTEM) {
        if (xQueueSendToFront(xQueueBase, &packet, pdMS_TO_TICKS(100)) == pdTRUE) {
            return true;
        }
        Serial.println("[BASE] Queue quase cheia!");
        return false;
    }

    if (xQueueSend(xQueueBase, &packet, pdMS_TO_TICKS(100)) == pdTRUE) {
        return true;
    }

    Serial.println("[BASE] Queue quase cheia!");
    return false;
}

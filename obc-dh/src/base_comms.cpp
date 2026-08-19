#include "base_comms.h"
#include <HardwareSerial.h>

HardwareSerial SerialPLD(2);

static QueueHandle_t xQueueBase = NULL;

static TaskHandle_t xBaseTask = NULL;

// BASE TASK
static void Base_Task(void* parameter)
{
    LogPacket packet;

    Serial.println("[BASE TASK] Iniciada.");

    while (true)
    {
        if (xQueueReceive(
                xQueueBase,
                &packet,
                portMAX_DELAY
            ) == pdTRUE)
        {
            SerialPLD.print(packet.data);
            SerialPLD.print(",");

            SerialPLD.print(packet.time);
            SerialPLD.print(",");

            SerialPLD.print(packet.type);
            SerialPLD.print(",");

            SerialPLD.println(packet.payload);
        }
    }
}

// INICIALIZAÇÃO
bool Base_Init()
{
    SerialPLD.begin(
        BASE_BAUDRATE,
        SERIAL_8N1,
        PIN_RXpld,
        PIN_TXpld
    );

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
        1
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
bool Base_SendPacket(const LogPacket& packet)
{
    if (xQueueBase == NULL)
    {
        return false;
    }

    if (xQueueSend(
            xQueueBase,
            &packet,
            pdMS_TO_TICKS(100)
        ) != pdTRUE)
    {
        Serial.println(
            "[BASE] Queue cheia!"
        );

        return false;
    }

    return true;
}

#include "rtc.h"
#include <RTClib.h>

// Instância do sensor RTC I2C
static RTC_DS3231 rtc;

bool RTC_Init() {
    // Inicializa a comunicação com o DS3231 no barramento Wire ativo
    if (!rtc.begin()) {
        Serial.println("[RTC] Não foi possível encontrar o módulo RTC!");
        return false;
    }

    // Verifica se o RTC perdeu a energia e precisa de ajuste de hora inicial
    if (rtc.lostPower()) {
        Serial.println("[RTC] Alerta: RTC perdeu energia! Ajustando hora padrão do compilador...");
        // Define a data/hora inicial para a data em que o código foi compilado
        rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    }

    return true;
}

bool RTC_SetDateTime(const RTC_DateTime &dateTime) {
    rtc.adjust(DateTime(
        dateTime.year,
        dateTime.month,
        dateTime.day,
        dateTime.hour,
        dateTime.minute,
        dateTime.second
    ));
    return true;
}

RTC_DateTime RTC_GetDateTime() {
    DateTime now = rtc.now();
    RTC_DateTime dt;

    dt.year   = now.year();
    dt.month  = now.month();
    dt.day    = now.day();
    dt.hour   = now.hour();
    dt.minute = now.minute();
    dt.second = now.second();

    return dt;
}

uint32_t RTC_GetUnixTimestamp() {
    DateTime now = rtc.now();
    return now.unixtime();
}
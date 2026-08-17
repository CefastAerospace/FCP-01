#ifndef RTC_H
#define RTC_H

#include <Arduino.h>
#include <Wire.h>

// Estrutura para armazenamento e leitura de data/hora
struct RTC_DateTime {
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
};

// Protótipos das funções do RTC
bool RTC_Init();
bool RTC_SetDateTime(const RTC_DateTime &dateTime);
RTC_DateTime RTC_GetDateTime();
uint32_t RTC_GetUnixTimestamp();

#endif // RTC_H
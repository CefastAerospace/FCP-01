#ifndef PAYLOAD_H
#define PAYLOAD_H

#include <Arduino.h>

// Protótipos das Funções
bool Payload_Init();
void Payload_SendData(const char* data);
bool Payload_ReadResponse(char* buffer, size_t maxLen);

#endif // PAYLOAD_H
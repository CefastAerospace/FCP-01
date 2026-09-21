#ifndef TTC_SELFTEST_H
#define TTC_SELFTEST_H

// Bateria de autoteste do TT-C usando o LoRaMock.
// Só existe no env esp32dev-mock (TTC_USE_MOCK).
// Uso: digitar "ttc test" no Monitor Serial.
#ifdef TTC_USE_MOCK
void TTC_SelfTest_Run();
#endif

#endif // TTC_SELFTEST_H

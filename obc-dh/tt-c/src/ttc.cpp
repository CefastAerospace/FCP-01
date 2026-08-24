#include "ttc.h"
#include "rtc/src/rtc.h" // Interface com o RTC

bool TTC_HandleSetTimeCommand(const TC_SetTime &cmd) {
    if (cmd.command_id != CMD_SET_TIME) {
        return false;
    }

    // TODO: Adicionar verificação de CRC16 no cmd.checksum se necessário

    // Aplica o timestamp recebido no RTC
    RTC_SetTime(cmd.unix_timestamp);
    
    Serial.printf("[TT&C] RTC sincronizado via TC_SET_TIME: %u\n", cmd.unix_timestamp);
    return true;
}
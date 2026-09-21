#ifdef TTC_USE_MOCK

#include "ttc_selftest.h"
#include "ttc.h"
#include "../../src/drivers/lora_mock.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// Handle da TaskTTC (definido no main.cpp) — suspenso durante o teste
extern TaskHandle_t xTaskTTC_Handle;

static int g_pass = 0;
static int g_fail = 0;

static void check(const char* nome, bool ok) {
    if (ok) { g_pass++; Serial.printf("  [PASS] %s\n", nome); }
    else    { g_fail++; Serial.printf("  [FAIL] %s\n", nome); }
}

// Monta um telecomando com CRC válido
static void build_tc(TelecommandPacket_t* pkt, uint16_t seq, uint8_t cmd,
                     const uint8_t* args, size_t args_len) {
    memset(pkt, 0, sizeof(TelecommandPacket_t));
    pkt->sequence_id = seq;
    pkt->command_id = cmd;
    pkt->flags = 0;
    pkt->timestamp = 0;
    if (args != NULL && args_len > 0) {
        size_t n = (args_len > sizeof(pkt->arguments)) ? sizeof(pkt->arguments) : args_len;
        memcpy(pkt->arguments, args, n);
    }
    pkt->checksum = TTC.CalculateCRC16((const uint8_t*)pkt, sizeof(TelecommandPacket_t) - sizeof(uint16_t));
}

// Injeta um TC e processa (1 ciclo de TaskUpdate). Retorna bytes do último TX.
static size_t inject_and_run(const TelecommandPacket_t* pkt) {
    LoRaMock* mock = TTC_GetMock();
    mock->InjectPacket((const uint8_t*)pkt, sizeof(TelecommandPacket_t));
    TTC.TaskUpdate();
    size_t len = 0;
    mock->GetLastTx(&len);
    return len;
}

// Valida um ACK no buffer do mock. Retorna status_code ou -1 se inválido.
static int read_ack(uint8_t* out_status) {
    size_t len = 0;
    const uint8_t* buf = TTC_GetMock()->GetLastTx(&len);
    if (len != sizeof(ACKPacket_t)) return -1;
    ACKPacket_t ack;
    memcpy(&ack, buf, sizeof(ACKPacket_t));
    if (ack.header != 0xAA55) return -1;
    uint16_t crc = TTC.CalculateCRC16(buf, sizeof(ACKPacket_t) - sizeof(uint16_t));
    if (crc != ack.checksum) return -1;
    if (out_status != NULL) *out_status = ack.status_code;
    return ack.status_code;
}

// Lê o último pacote TM transmitido e valida. Retorna true se íntegro.
static bool read_tm(TelemetryPacket_t* out) {
    size_t len = 0;
    const uint8_t* buf = TTC_GetMock()->GetLastTx(&len);
    if (len != sizeof(TelemetryPacket_t)) return false;
    memcpy(out, buf, sizeof(TelemetryPacket_t));
    if (out->header != TM_HEADER) return false;
    if (out->packet_type != TM_TYPE_TELEMETRY) return false;
    uint16_t crc = TTC.CalculateCRC16(buf, sizeof(TelemetryPacket_t) - sizeof(uint16_t));
    return (crc == out->checksum);
}

static void read_link_stats(uint32_t* rx, uint32_t* tx, uint32_t* err) {
    uint8_t buf[64];
    size_t out_len = 0;
    if (TTC.GetTelemetry(buf, sizeof(buf), &out_len) == SUBSYS_OK && out_len >= sizeof(TTC_Telemetry_t)) {
        TTC_Telemetry_t* t = (TTC_Telemetry_t*)buf;
        if (rx != NULL) *rx = t->rx_packets_count;
        if (tx != NULL) *tx = t->tx_packets_count;
        if (err != NULL) *err = t->rx_errors_count;
    }
}

void TTC_SelfTest_Run() {
    g_pass = 0;
    g_fail = 0;
    Serial.println("\n=== AUTOTESTE TT-C (mock) ===");

    if (!TTC.HealthCheck()) {
        Serial.println("[SELFTEST] Radio mock nao pronto, abortando.");
        return;
    }

    // Congela a TaskTTC para não disputar o módulo durante o teste
    if (xTaskTTC_Handle != NULL) vTaskSuspend(xTaskTTC_Handle);

    System_SetState(STATE_MISSION);
    TelecommandPacket_t pkt;
    uint8_t status = 0xFF;

    // ---- T1: PING válido ----
    build_tc(&pkt, 0x1234, CMD_PING, NULL, 0);
    inject_and_run(&pkt);
    {
        uint8_t st = 0xFF;
        size_t len = 0;
        const uint8_t* buf = TTC_GetMock()->GetLastTx(&len);
        bool ok = (read_ack(&st) == ACK_OK) && (st == ACK_OK);
        if (ok) {
            ACKPacket_t ack;
            memcpy(&ack, buf, sizeof(ACKPacket_t));
            ok = (ack.sequence_id == 0x1234);
        }
        check("T1 PING -> ACK_OK com sequence ecoado", ok);
    }

    // ---- T2: CRC corrompido ----
    {
        uint32_t rx0, tx0, err0;
        read_link_stats(&rx0, &tx0, &err0);
        build_tc(&pkt, 0x1235, CMD_PING, NULL, 0);
        pkt.arguments[0] ^= 0xFF;  // corrompe após calcular o CRC
        inject_and_run(&pkt);
        uint32_t rx1, tx1, err1;
        read_link_stats(&rx1, &tx1, &err1);
        check("T2 CRC inválido -> NACK ERR_CRC + contador de erro",
              (read_ack(&status) == ERR_CRC) && (err1 == err0 + 1));
    }

    // ---- T3: framing da telemetria ----
    {
        TTC.SendTelemetryPacket();
        TTC.TaskUpdate();
        TelemetryPacket_t tm1, tm2;
        bool ok1 = read_tm(&tm1);
        TTC.SendTelemetryPacket();
        TTC.TaskUpdate();
        bool ok2 = read_tm(&tm2);
        check("T3 TM íntegro + sequence incremental",
              ok1 && ok2 && (uint16_t)(tm2.sequence_id - tm1.sequence_id) == 1);
    }

    // ---- T4: ENTER/EXIT_SAFE ----
    System_SetState(STATE_MISSION);
    build_tc(&pkt, 0x2001, CMD_ENTER_SAFE, NULL, 0);
    inject_and_run(&pkt);
    check("T4a ENTER_SAFE -> ACK + estado SAFE",
          (read_ack(&status) == ACK_OK) && (System_GetState() == STATE_SAFE));
    {
        uint8_t badkey[2] = {0x00, 0x00};
        build_tc(&pkt, 0x2002, CMD_EXIT_SAFE, badkey, 2);
        inject_and_run(&pkt);
        check("T4b EXIT_SAFE chave errada -> ERR_INVALID_PARAM, segue SAFE",
              (read_ack(&status) == ERR_INVALID_PARAM) && (System_GetState() == STATE_SAFE));
    }
    {
        uint8_t goodkey[2] = {0x5A, 0xA5};  // 0xA55A little-endian
        build_tc(&pkt, 0x2003, CMD_EXIT_SAFE, goodkey, 2);
        inject_and_run(&pkt);
        check("T4c EXIT_SAFE chave certa -> ACK + estado MISSION",
              (read_ack(&status) == ACK_OK) && (System_GetState() == STATE_MISSION));
    }

    // ---- T5: SET_TIME ----
    {
        uint32_t unix = 1700000000UL;
        build_tc(&pkt, 0x3001, CMD_SET_TIME, (uint8_t*)&unix, sizeof(unix));
        inject_and_run(&pkt);
        bool ok = (read_ack(&status) == ACK_OK);
        TTC.SendTelemetryPacket();
        TTC.TaskUpdate();
        TelemetryPacket_t tm;
        ok = ok && read_tm(&tm) && (tm.timestamp >= unix) && (tm.timestamp <= unix + 10);
        check("T5 SET_TIME -> timestamp Unix no TM seguinte", ok);
    }

    // ---- T6: comandos internos ----
    {
        SubsystemCommand_t c = {};
        c.command_id = TTC_CMD_TX_TELEMETRY;
        check("T6a interno TX_TELEMETRY -> SUBSYS_OK",
              TTC.HandleCommand(c) == SUBSYS_OK);
        c.command_id = TTC_CMD_BEACON_ON;
        check("T6b interno BEACON_ON -> SUBSYS_OK",
              TTC.HandleCommand(c) == SUBSYS_OK);
        c.command_id = TTC_CMD_SET_BEACON_INT;
        uint32_t bad_int = 500;
        memcpy(c.payload, &bad_int, sizeof(bad_int));
        c.payload_len = sizeof(bad_int);
        check("T6c interno SET_BEACON_INT 500ms -> PARAM_INVALID",
              TTC.HandleCommand(c) == SUBSYS_ERR_PARAM_INVALID);
        uint32_t good_int = 60000;
        memcpy(c.payload, &good_int, sizeof(good_int));
        c.command_id = TTC_CMD_SET_BEACON_INT;
        check("T6d interno SET_BEACON_INT 60s -> SUBSYS_OK",
              TTC.HandleCommand(c) == SUBSYS_OK);
        c.command_id = TTC_CMD_BEACON_OFF;
        check("T6e interno BEACON_OFF -> SUBSYS_OK",
              TTC.HandleCommand(c) == SUBSYS_OK);
        c.command_id = TTC_CMD_TX_RAW;
        const char* hello = "HELLO";
        memcpy(c.payload, hello, 5);
        c.payload_len = 5;
        bool ok = (TTC.HandleCommand(c) == SUBSYS_OK);
        TTC.TaskUpdate();
        size_t txlen = 0;
        const uint8_t* txb = TTC_GetMock()->GetLastTx(&txlen);
        ok = ok && (txlen == 5) && (memcmp(txb, "HELLO", 5) == 0);
        check("T6f interno TX_RAW -> bytes entregues ao radio", ok);
        c.command_id = TTC_CMD_RESET_STATS;
        ok = (TTC.HandleCommand(c) == SUBSYS_OK);
        uint32_t rx, tx, err;
        read_link_stats(&rx, &tx, &err);
        check("T6g interno RESET_STATS -> contadores zerados",
              ok && (rx == 0) && (tx == 0) && (err == 0));
    }

    // ---- T7: DEPLOY_ANTENNA não-bloqueante ----
    System_SetState(STATE_SAFE);
    {
        uint8_t arg = 1;
        build_tc(&pkt, 0x4001, CMD_DEPLOY_ANTENNA, &arg, 1);
        inject_and_run(&pkt);
        check("T7a DEPLOY fora de MISSION -> ERR_INVALID_CMD",
              read_ack(&status) == ERR_INVALID_CMD);
    }
    System_SetState(STATE_MISSION);
    {
        uint8_t arg = 1;
        build_tc(&pkt, 0x4002, CMD_DEPLOY_ANTENNA, &arg, 1);
        inject_and_run(&pkt);
        bool ok = (read_ack(&status) == ACK_OK);
        ok = ok && (digitalRead(33) == HIGH);  // pino ligado sem travar
        // Aguarda a queima terminar sozinha (~1 s) sem bloquear o resto
        uint32_t t0 = millis();
        while (digitalRead(33) == HIGH && (millis() - t0) < 3000) {
            TTC.TaskUpdate();
            vTaskDelay(pdMS_TO_TICKS(50));
        }
        ok = ok && (digitalRead(33) == LOW);
        check("T7b DEPLOY em MISSION -> ACK imediato + pino desliga sozinho", ok);
    }

    // ---- T8: RESET_OBC (manual) ----
    Serial.println("  [SKIP] T8 RESET_OBC -> manual: reinicia a placa (testar por último)");

    if (xTaskTTC_Handle != NULL) vTaskResume(xTaskTTC_Handle);

    Serial.printf("=== RESULTADO: %d PASS, %d FAIL ===\n\n", g_pass, g_fail);
}

#endif // TTC_USE_MOCK

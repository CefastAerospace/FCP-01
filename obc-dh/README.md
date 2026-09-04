# OBC&DH — On-Board Computer & Data Handling

Firmware do subsistema **OBC&DH** (*On-Board Computer and Data Handling*) desenvolvido pela equipe **CEFAST Aerospace** para a missão **CubeDesign 2026**.

O subsistema é responsável pelo gerenciamento central do CubeSat, executando o controle de estados (*ConOps*), coordenação de subsistemas (EPS, ADCS, TT&C, Payload), processamento de dados e interface de diagnósticos em tempo real.

---

## 📌 Visão Geral

O projeto utiliza o **ESP32 DevKit V1** como unidade central de processamento, operando sob o sistema operacional de tempo real **FreeRTOS** com distribuição explícita de tarefas entre os dois núcleos da MCU (*Dual-Core Pinning*).

* **Arquitetura de Referência:** *CubeDesign 2026 — OBC&DH: Definição de Arquitetura v1.5* (10/07/2026).
* **Conformidade:** *CubeSat Design Specification (CDS) v14.1*.

---

## 🛰️ Especificações de Hardware & Interface (Pinout)

* **MCU:** ESP32 DevKit V1 (Dual-Core, Xtensa® 32-bit LX6)
* **Arquitetura de Firmware:** C/C++ sobre FreeRTOS
* **Módulo TT&C:** LoRa SX1276 (Interface VSPI)
* **Payload:** Raspberry Pi Zero W / ADS-B (Comunicação Serial UART)
* **RTC:** DS3231 (Interface I²C)

### Mapeamento de Pinos

| Periférico / Função | Interface | Pino (GPIO) | Descrição |
| :--- | :--- | :--- | :--- |
| **LED_BUILTIN** | GPIO | GPIO 2 | Indicador de status visual |
| **I²C (SDA / SCL)** | I²C | GPIO 21 / 22 | Interface para RTC, EPS, TC e ADCS |
| **LoRa SX1276 (SCK, MISO, MOSI, CS)** | VSPI | GPIO 18, 19, 23, 5 | Barramento SPI para Telecomunicações (TT&C) |
| **LoRa (RESET / DIO0)** | GPIO | GPIO 14 / 4 | Pinos de controle e interrupção do LoRa |
| **PWM (Fases U, V, W)** | PWM | GPIO 25, 26, 27 | Atuadores do ADCS (SimpleFOC) |
| **Motor Enable** | GPIO | GPIO 32 | Habilitação do driver do motor |
| **Burn Wire** | GPIO | GPIO 33 | Gatilho para mecanismo de implantação |

---

## ⚙️ Arquitetura de Software & Concorrência (FreeRTOS)

O firmware distribui as responsabilidades em *Tasks* otimizadas e vinculadas estrategicamente a núcleos específicos para garantir determinismo e isolamento de falhas.

### Distribuição de Tarefas por Core

* **Core 1 (`CORE_CONTROL`) — Controle e Malhas Críticas:**
  * `TaskTTC` (10 Hz / Prioridade 3): Execução e atualização das rotinas de telecomunicação.
  * `TaskADCS` (20 Hz / Prioridade 3): Algoritmos e malha de controle de atitude.

* **Core 0 (`CORE_IO`) — Monitoramento, Comunicação e Estado:**
  * `TaskEPS` (2 Hz / Prioridade 2): Leitura de telemetria de energia e verificação de saúde (*HealthCheck*).
  * `TaskConOps` (1 Hz / Prioridade 1): Supervisão do estado geral do sistema e aplicação de ações preventivas (ex: transição forçada para `STATE_SAFE`).
  * `TaskPayload` (Prioridade 2): Processamento do protocolo UART binário e gerência do link com a Raspberry Pi Zero W.

### Sincronização & Inicialização
* **EventGroup & Subsystem Readiness:** Durante o `setup()`, cada subsistema sinaliza sua prontidão em um *EventGroup* dedicado (`SYS_BIT_EPS_READY`, `SYS_BIT_ADCS_READY`, etc.). O sistema aguarda até 5 segundos por todos os módulos críticos antes de transitar para `STATE_MISSION`.

---

## 🕹️ Modos Operacionais (ConOps) & Interface CLI

O gerenciamento de modos opera via máquina de estados:
* **`STATE_BOOT_CHECK`:** Verificação inicial de integridade dos periféricos e subsistemas.
* **`STATE_MISSION`:** Modo nominal de operação com todas as tarefas ativas.
* **`STATE_SAFE`:** Modo de segurança acionado em caso de falhas na inicialização ou degradação do EPS (baixa tensão/superaquecimento).

### Terminal de Comando e Diagnóstico (Serial)
Através da interface Serial (115200 baud), o operador dispõe de comandos em tempo real:
* `1`, `2`, `3`: Força transição entre `STATE_MISSION`, `STATE_SAFE` e `STATE_BOOT_CHECK`.
* `rpm <valor>` / `stop` / `estop`: Ajuste de rotação, parada e parada de emergência do motor ADCS.
* `payload start` / `stop` / `reboot`: Controle da coleta de dados e ciclo de energia da Raspberry Pi.
* `s`: Imprime o status da telemetria, baterias, motor ADCS e contadores ADS-B.
* `r`: Executa a reinicialização (*Reboot*) do ESP32.

---

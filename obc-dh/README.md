# OBC&DH — On-Board Computer & Data Handling

Firmware do subsistema **OBC&DH** (*On-Board Computer and Data Handling*) desenvolvido pela equipe **Cefast Aerospace** para a missão **CubeDesign 2026**.

O subsistema é responsável pelo gerenciamento e controle do computador de bordo do CubeSat, abrangendo o processamento de dados, execução da máquina de estados (modos operacionais), comunicação inter-subsistemas, armazenamento de dados e monitoramento de saúde (*housekeeping*).

---

## 📌 Visão Geral

O projeto utiliza o **ESP32 DevKit V1** como unidade central de processamento, operando sob o sistema operacional de tempo real **FreeRTOS** para execução concorrente de tarefas.

* **Arquitetura de Referência:** *CubeDesign 2026 — OBC&DH: Definição de Arquitetura v1.5* (10/07/2026).
* **Conformidade:** *CubeSat Design Specification (CDS) v14.1*.

---

## 🛰️ Especificações de Hardware

* **MCU:** ESP32 DevKit V1 (Dual-Core, Xtensa® 32-bit LX6)
* **Arquitetura de Firmware:** C/C++ sobre FreeRTOS
* **Armazenamento Persistente:** Cartão MicroSD (Interface HSPI)
* **Relógio em Tempo Real (RTC):** DS3231 (Interface I²C)
* **Telemetria/Comunicação:** Módulo LoRa / TT&C (Interface VSPI)

### Mapeamento de Pinos (Pinout)

| Periférico / Função | Interface | Pino (GPIO) | Observação |
| :--- | :--- | :--- | :--- |
| **LED_BUILTIN** | GPIO | GPIO 2 | Status visual |
| **RPi Zero W (RX / TX)** | UART | GPIO 16 / 17 | Comunicação Payload/PLD |
| **I²C (SDA / SCL)** | I²C | GPIO 21 / 22 | RTC, EPS, ADCS, TC |
| **LoRa (SCK, MISO, MOSI, CS)** | VSPI | GPIO 18, 19, 23, 5 | Telemetria / TT&C |
| **LoRa (RESET / DIO0)** | GPIO | GPIO 14 / 4 | Controle de interrupção/reset |
| **MicroSD (SCK, MISO, MOSI, CS)** | HSPI | GPIO 15, 35, 12, 13 | GPIO 35 configurado apenas como Input |
| **PWM (Fases U, V, W)** | PWM | GPIO 25, 26, 27 | Atuadores / Controle |
| **Motor Enable** | GPIO | GPIO 32 | Habilitação de atuador |
| **Burn Wire** | GPIO | GPIO 33 | Mecanismo de implantação |

---

## ⚙️ Arquitetura de Software & Recursos FreeRTOS

O firmware distribui as responsabilidades em tarefas (*Tasks*) independentes, utilizando abordagens de baixo acoplamento e concorrência segura.

### Recursos do Kernel Utilizados
* **Multi-threading & Core Affinity:** Distribuição de *Tasks* entre os dois núcleos do ESP32.
* **Queues (Filas):** Transmissão segura de mensagens e telemetria entre tarefas.
* **Sincronização:** Utilização de *Mutexes* para proteção de barramentos compartilhados (I²C/SPI) e *Binary Semaphores* para sincronização por eventos.
* **Event Groups:** Sinalização de múltiplos estados do sistema.
* **Software Timers:** Agendamento de rotinas periódicas e temporizações estendidas de missão.
* **ISRs e APIs `FromISR`:** Interrupções otimizadas repassando o processamento pesado para as *Tasks* correspondentes.

### Estrutura de Tasks Implementadas
* **OBC Task:** Coordenação central e gerenciamento dos modos operacionais.
* **Sensor / Data Task:** Aquisição e tratamento de dados dos sensores e subsistemas.
* **Telemetry Task:** Formatação e empacotamento das telemetrias.
* **SD Logger Task:** Gravação assíncrona de logs e dados de voo na mídia flash.
* **Base Communication Task:** Transmissão e recepção de pacotes via LoRa/UART.

---

## 🛡️ Tolerância a Falhas e Diagnóstico

* **Monitoramento de Memória (Heap Caps):** Mapeamento constante do consumo de RAM dinâmica (`esp_heap_caps.h`) categorizado nos estados `MEMORY_OK`, `MEMORY_WARNING` e `MEMORY_CRITICAL`.
* **Monitoramento de Stack:** Verificação preventiva do consumo de pilha por tarefa para mitigar *Stack Overflow*.
* **Watchdog Timer (WDT):** Proteção contra travamentos de software e execução de estratégias de recuperação em caso de falha.
* **Desacoplamento de I/O:** Escritas no Cartão SD e transmissões de rádio são isoladas via *Queues*, evitando bloqueios na *OBC Task*.

│   ├── eps.h
│   └── eps.cpp
└── README.md

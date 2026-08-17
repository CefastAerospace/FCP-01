# FCP-01: Falcão-Peregrino

CubeSat 2U desenvolvido pela CEFAST Aerospace (equipe aeroespacial do CEFET-MG) para o CubeDesign 2026, competição que acontece em novembro em Buenos Aires, Argentina.

O FCP-01 é o primeiro satélite da linha Falcão-Peregrino, a linha de CubeSats 2U da equipe. As demais linhas são Magela (CubeSats 1U) e Harpia (CanSats).

A missão do FCP-01 é a recepção e o processamento de sinais **ADS-B**, usados para identificação e monitoramento de aeronaves.

Este repositório é **privado** e contém o código de desenvolvimento do satélite. Documentação de engenharia completa (requisitos, arquitetura, resultados de testes) fica no Google Drive da equipe: [Link do Drive](https://drive.google.com/drive/folders/1zuqoZDisrKwNKC3qj_JZJbrycwYCYJCP).

## Arquitetura

O FCP-01 usa uma arquitetura **centralizada**: um único ESP32 (OBC) roda **FreeRTOS** e gerencia todos os subsistemas via periféricos ligados diretamente a ele. O **Payload** é a exceção, roda em um Raspberry Pi Zero W separado, e se comunica com o OBC por UART.

| Subsistema | Hardware | Interface com o OBC |
|---|---|---|
| **OBC & Data Handling** | ESP32, cartão SD, RTC | - (é o próprio OBC) |
| **TT&C** | LoRa SX1276 (915 MHz), servo motor de antena | SPI (HSPI) + PWM |
| **EPS** | 2x INA219 | I2C |
| **Thermal Control** | LM75A | I2C |
| **ADCS** | SimpleFOC Mini (roda de reação) + MPU6050 | PWM/analógico + I2C |
| **Payload** | RTL-SDR v3 + Raspberry Pi Zero W | UART |
| **STR** | CAD (SolidWorks) | - |

O ESP32 usa os dois barramentos SPI disponíveis: **VSPI** para o cartão SD e **HSPI** para o LoRa.

## Estrutura do repositório

```text
FCP-01/
├── README.md
├── .gitignore
├── obc-dh/              # ÚNICO firmware real - platformio.ini mora aqui
│   ├── platformio.ini
│   ├── src/
│   │   ├── main.cpp     # setup do FreeRTOS e criação das tasks
│   │   ├── tasks/        # uma task por subsistema (adcs, eps, ttc, payload_link...)
│   │   └── drivers/      # periféricos do próprio OBC (sd_card, rtc)
│   └── include/
├── adcs/                 # biblioteca - consumida pelo obc-dh via lib_extra_dirs
│   ├── include/
│   └── src/
├── eps-tc/               # biblioteca - EPS e Thermal Control
│   ├── include/
│   └── src/
├── tt-c/                 # biblioteca - LoRa + controle do servo de antena
│   ├── include/
│   └── src/
├── payload/               # Raspberry Pi Zero W - projeto separado, NÃO é PlatformIO
│   └── (ex: requirements.txt, main.py)
├── str/
│   ├── exports/            # arquivos .STEP
│   ├── drawings/           # desenhos técnicos em .PDF
│   └── manufacturing/
└── common/
    ├── protocol/            # spec do protocolo UART (OBC ↔ Payload) e pacotes TT&C
    └── interfaces/
        └── pinout.md        # endereços I2C, alocação de pinos e barramentos SPI
```

**Importante:** `adcs/`, `eps-tc/` e `tt-c/` não são firmwares que rodam sozinhos: são bibliotecas que só ganham vida quando compiladas junto com `obc-dh/`. Só `obc-dh/` (no ESP32) e `payload/` (no Raspberry Pi) são projetos que rodam de forma independente.

## Fluxo de desenvolvimento (Git)

Fluxo padrão para mudanças dentro do seu próprio subsistema:

```bash
git pull
git status
# edite os arquivos
git add .
git commit -m "nome do subsistema: descrição da alteração"
git push
```

Os nomes dos subsistemas devem seguir o padrão de nomenclatura das pastas do repositório.

A `main` está protegida contra force-push e deleção, mas não exige review obrigatório.

## Bibliotecas e dependências

Dependências de terceiros (ESP32/Arduino) são declaradas em `obc-dh/platformio.ini` via `lib_deps`, com versão fixada. Código interno de cada subsistema é integrado via `lib_extra_dirs`, apontando para as pastas irmãs (`adcs/`, `eps-tc/`, `tt-c/`, `common/`). Nunca copie bibliotecas de terceiros para dentro do repositório.

## CAD

Arquivos nativos do SolidWorks (`.SLDPRT`, `.SLDASM`, `.SLDDRW`) **não** entram no repositório, ficam no Google Drive. Apenas exports em `.STEP` (`str/exports/`) e desenhos em `.PDF` (`str/drawings/`) são versionados aqui.

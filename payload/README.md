# PAYLOAD — ADS-B Air Surveillance System

Software de carga útil (*Payload*) desenvolvido pela equipe **CEFAST Aerospace** para a missão **CubeDesign 2026**.

O subsistema gerencia a telemetria da missão, extraindo parâmetros de trajetória em tempo real para o computador de bordo e a estação de solo.

---

## 📌 Visão Geral

O script `telemetry.py` opera de forma autônoma coletando fluxos TCP na porta `30003` do `dump1090`, processando mensagens baseadas em estruturas tipadas (`dataclasses`) e garantindo conformidade com os requisitos da missão simulada.

* **Arquitetura de Referência:** *CubeDesign 2026 — ADS-B Mission for Air Surveillance in Remote Areas*.
* **Conformidade de Dados:** Geração de telemetria estruturada com carimbos temporais em UTC (ISO 8601), controle de sequência e validações de integridade para reconstrução de trajetórias.

---

## 🛰️ Especificações Técnicas & Parâmetros (HLR-ADS-02)

O sistema decodifica e armazena os seguintes parâmetros obrigatórios da telemetria de tráfego aéreo estruturados via *dataclass*:

| Parâmetro | Tipo | Descrição |
| :--- | :--- | :--- |
| **`icao`** | String | Identificador ICAO exclusivo da aeronave |
| **`alt`** | Float | Altitude barométrica em pés (ft) |
| **`vel`** | Float | Velocidade em relação ao solo em nós (kt) |
| **`heading`** | Float | Proa / Direção de deslocamento (Track Angle) em graus |
| **`lat` / `lon`** | Float | Posição geográfica (Latitude e Longitude) |
| **`timestamp`** | String | Timestamp em UTC (ISO 8601) |
| **`msg_type`** | String | Tipo da mensagem decodificada do fluxo |
| **`val_flags`** | Boolean | Flag indicando se a telemetria possui dados válidos mínimos |
| **`seq_id`** | Integer | ID sequencial incremental para controle de pacotes |

---

## 📂 Estrutura do Log de Missão (JSON Lines)

Os dados processados são salvos incrementalmente no arquivo padrão em `/mnt/data/mission_log.json`:

```json
{"icao":"E4953A","timestamp":"2026-09-08T00:21:44Z","lat":-19.95438,"lon":-43.6116,"alt":39000.0,"vel":489.0,"heading":37.0,"msg_type":"3","val_flags":true,"seq_id":12}
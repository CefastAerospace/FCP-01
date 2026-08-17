# Pinout e Alocação de Barramentos: FCP-01

Fonte única de verdade para endereços I2C, barramentos SPI e pinos reservados. Atualizar sempre que um endereço/pino for definido ou mudar. Evita que dois subsistemas briguem pelo mesmo recurso só na hora de integrar o hardware.

## Barramentos SPI (ESP32)

| Barramento | Dispositivo | Subsistema |
|---|---|---|
| VSPI | Cartão SD | OBC&DH |
| HSPI | LoRa SX1276 | TT&C |

## Barramento I2C (compartilhado)

| Endereço | Dispositivo | Subsistema | Observação |
|---|---|---|---|
| 0x68 (padrão) | RTC | OBC&DH | ⚠️ mesmo endereço padrão do MPU6050, ver conflito abaixo |
| 0x69 | MPU6050 | ADCS | AD0 amarrado em VCC para evitar colisão com o RTC (padrão é 0x68) |
| 0x40 | INA219 #1 | EPS | endereço definido pelos pinos A0/A1 |
| 0x41 | INA219 #2 | EPS | A0=VCC, A1=GND (ajustar conforme datasheet) |
| 0x48 | LM75A | TC | endereço definido pelos pinos A0–A2 |

⚠️ **Conflito a confirmar:** se o RTC for DS3231 ou DS1307 (endereço fixo 0x68), o MPU6050 precisa ir para 0x69 (AD0 em VCC), como já refletido na tabela acima. Confirmar o modelo exato do RTC para fechar isso. Se for um módulo com endereço diferente (ex: PCF8563, 0x51), não há conflito e o MPU6050 pode ficar no padrão 0x68.

## Outros pinos reservados

| Recurso | Subsistema | Tipo | Observação |
|---|---|---|---|
| Servo motor de antena | TT&C | PWM | pino dedicado, fora do SPI |
| SimpleFOC Mini (roda de reação) | ADCS | PWM (3 fases) + corrente/encoder | confirmar pinos exatos conforme datasheet do driver |

## Como manter

Qualquer subsistema que adicionar um novo módulo I2C/SPI ou reservar um pino deve atualizar esta tabela antes de integrar o hardware físico com o resto do satélite.

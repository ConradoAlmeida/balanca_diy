# Hardware

[← Voltar ao README](../README.md) · [Bateria](BATERIA.md) · [Firmware](FIRMWARE.md)

> Componentes, pinagem, wiring, display OLED e diagrama geral.

---

## Montagem

### Componentes

| Componente | Modelo | Qty |
|------------|--------|-----|
| Microcontrolador | ESP32 DevKit V1/V4 (ESP32-WROOM-32, 4MB Flash) | 1 |
| Célula de carga + ADC | HX711 | 1 |
| Display | OLED 0.96" SSD1306 (I2C, 128x64) | 1 |
| Botões | Push button momentâneo (6x6mm) | 2 |
| Módulo de carga | IP5306 (1S Li-ion, 5V boost) | 1 |
| Bateria | Li-ion 18650 (1S, 3.7V) | 1 |
| Conector | USB Type-C fêmea (com cabo, soldado) | 1 |
| Chave | Liga-desliga SPST | 1 |
| Resistores CC | 5.1kΩ 1% (obrigatório para USB-C PD) | 2 |
| Resistores Divisor | 100kΩ 1% (monitoramento de bateria) | 2 |

### Pinagem

| GPIO | Função | Detalhe |
|------|--------|---------|
| 23 | HX711 DOUT | Dados do ADC (input) |
| 22 | HX711 SCK | Clock do ADC (output) |
| 21 | OLED SDA | I2C hardware data |
| 19 | OLED SCL | I2C hardware clock |
| 34 | ADC Battery | Monitor de tensão da bateria (input only) |
| 4 | Botão A | Uso geral (pull-up interno) |
| 15 | Botão B | Funções extras (pull-up interno) |

### Wiring Detalhado

#### HX711 + Célula de Carga

```
Célula de Carga (5-wire)
├── E+ (vermelho) ────┐
├── E- (preto)  ──────┤──→ HX711: E+, E-
├── A- (branco) ──────┤
├── A+ (verde)  ──────┘

HX711 Module
├── VCC ───────→ 5V (do IP5306 ou USB)
├── GND ───────→ GND
├── DT (DOUT) ──→ GPIO 23 (ESP32)
├── SCK ───────→ GPIO 22 (ESP32)
└── E+, E-, A+, A- → Célula de carga
```

**Notas:**
- A célula de carga é uma ponte de Wheatstone. A ordem das cores pode variar — consulte o datasheet do seu modelo.
- O HX711 opera com VCC de 2.6V a 5.5V. Para melhor precisão, alimente com 5V.
- Os sinais DT e SCK são 3.3V TTL — compatíveis diretamente com o ESP32 (não necessita level shifter).

#### OLED 0.96" (SSD1306 I2C)

```
OLED Module (4 pinos, endereço 0x3C)
├── VCC ───────→ 3.3V (ESP32)
├── GND ───────→ GND
├── SDA ───────→ GPIO 21 (ESP32)
└── SCL ───────→ GPIO 19 (ESP32)
```

**Notas:**
- O botão B alterna entre peso, histórico e bateria na mesma tela.
- Ao ligar, mantenha a plataforma vazia para que a tara automática seja correta.
- O clique curto do botão A registra a medição estável no histórico.
- Alimente a tela pelo 3.3V do ESP32.
- Se a tela não for detectada, verifique SDA=21, SCL=19 e a alimentação de 3.3V.

#### Monitoramento de Bateria (ADC)

```
Bateria (+) ────[R1: 100kΩ]────┬──── GPIO 34 (ESP32)
                               │
                               └────[R2: 100kΩ]──── GND
```

**Como funciona:**
- O divisor de tensão reduz a tensão da bateria pela metade para ficar dentro da faixa segura do ADC do ESP32 (0-3.3V).
- **Bateria cheia (4.2V):** GPIO 34 lê ~2.1V
- **Bateria vazia (3.0V):** GPIO 34 lê ~1.5V
- **Resistores de 100kΩ:** consomem pouquíssima corrente (~21µA), ideal para bateria.
- **GPIO 34:** É ADC1, input-only, e não conflita com os outros pinos usados.

**Conexão:** Ligue o divisor diretamente no terminal positivo da bateria (ou pino `BAT+` do IP5306) e o GND comum.

## Displays OLED

| Tela | Conteúdo |
|------|----------|
| Peso | Peso atual em gramas, em fonte grande, e estado da tara |
| Histórico | As 4 últimas medições, em gramas |
| Bateria | Tensão, percentual e alarme visual de bateria baixa |

A OLED usa o endereço `0x3C` no I2C hardware. O botão B alterna as três páginas na mesma tela.
O alarme de bateria baixa é visual e é acionado quando a tensão medida fica em `3.3V` ou menos (~8% na curva); o projeto não possui buzzer. Ver [Curva de percentual da bateria](BATERIA.md#curva-de-percentual-da-bateria) para a tabela completa.

## Diagrama Geral

```
┌─────────────────────────────────────────────────────┐
│                    USB Type-C                        │
│                  (cabo soldado)                      │
│                                                      │
│  CC1 ──┬── R 5.1kΩ ──→ GND                          │
│  CC2 ──┴── R 5.1kΩ ──→ GND                          │
└────────┬────────────────────────────────────────────┘
         │ 5V
         ▼
┌─────────────────────────────────────────────────────┐
│   IP5306                                            │
│  (charger/boost)│──→ 5V ──→ HX711 VCC               │
│                 │──→ chave ──→ ESP32 VIN            │
│  BAT+ ── 18650  │──→ Divisor (100k+100k) ──→ GPIO 34│
│  BAT- ── GND    │                                   │
└─────────────────┘

┌─────────────────────────────────────────────────────┐
│                  ESP32 DevKit V4                     │
│                                                      │
│  GPIO 23 ←── HX711 DT                               │
│  GPIO 22 ←→ HX711 SCK                               │
│  GPIO 21 ←→ OLED SDA (I2C hardware)                │
│  GPIO 19 ←→ OLED SCL (I2C hardware)                │
│  GPIO 34 ←── Divisor de Tensão (Bateria)            │
│  GPIO 4  ←── Botão A (GND)                          │
│  GPIO 15 ←── Botão B (GND)                          │
│                                                      │
│  WiFi: STA + AP                                     │
└─────────────────────────────────────────────────────┘
```

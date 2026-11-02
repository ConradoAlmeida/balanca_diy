# Bateria

[← Voltar ao README](../README.md) · [Hardware](HARDWARE.md) · [Firmware](FIRMWARE.md)

> Curva de percentual, tensão, alarme de bateria baixa e autonomia.

---

## Curva de percentual da bateria

O percentual exibido (OLED e web) segue a **curva típica de descarga Li-ion 1S**, baseada no perfil Silicon Lightworks: platô longo entre ~20% e ~80% (3,65–3,75 V), queda rápida no topo e no “joelho” abaixo de 20%.

**Regras:**
- Tensão **≥ 4,20 V** → **100%**
- Tensão **≤ 3,00 V** → **0%** (clamp; IP5306 perto de desligar o boost)
- Entre dois degraus → **interpolação linear**
- Calibração ADC: `BATTERY_CAL` em `config.h` (ajusta tensão medida, não o %)
- Alarme visual de bateria baixa: **≤ 3,30 V** (`BATTERY_LOW_VOLT`, ~8%)

**Tabela de degraus** (`BATTERY_CURVE` em `src/main.cpp`):

| Tensão (V) | % |
|------------|---|
| ≤ 3,00 | 0 |
| 3,00 | 0 |
| 3,20 | 5 |
| 3,35 | 10 |
| 3,50 | 15 |
| 3,65 | 20 |
| 3,67 | 25 |
| 3,68 | 30 |
| 3,69 | 35 |
| 3,70 | 40 |
| 3,71 | 45 |
| 3,72 | 50 |
| 3,73 | 55 |
| 3,74 | 60 |
| 3,75 | 70 |
| 3,80 | 80 |
| 3,85 | 90 |
| 4,00 | 95 |
| 4,20 | 100 |
| ≥ 4,20 | 100 |

**Exemplos interpolados:**

| Tensão (V) | % aprox. |
|------------|----------|
| 4,19 | 99 |
| 3,72 | 50 |
| 3,30 | 8 |
| 3,00 | 0 |

**Regiões da curva:**

| Faixa | Tensão | Comportamento |
|-------|--------|---------------|
| Topo | 4,00–4,20 V | Queda rápida (95→100%) |
| Platô | 3,65–3,75 V | ~20–70%; tensão quase estável |
| Joelho | 3,20–3,50 V | Queda acelerada (5→15%) |
| Piso | ≤ 3,00 V | Sempre 0% |

**Nota:** o IP5306 desliga a saída 5V por proteção quando a célula cai para ~2,8–2,9 V. O piso em 3,0 V = 0% reflete o limite prático de uso, não o esgotamento químico da célula (~2,2 V).

#### Botões Push Button

```
Botão A (GPIO 4)          Botão B (GPIO 15)
┌─────────────┐           ┌─────────────┐
│  o───────o  │           │  o───────o  │
│             │           │             │
│  o───────o  │           │  o───────o  │
└──────┬──────┘           └──────┬──────┘
       │                         │
       ├────→ GPIO 4             ├────→ GPIO 15
       │                         │
      GND                       GND
```

**Configuração:** Pull-up interno do ESP32 (`INPUT_PULLUP`). O botão conecta o pino ao GND quando pressionado. Não são necessários resistores externos.

#### Alimentação: Bateria Li-ion 1S + IP5306 + USB Type-C

```
USB Type-C (cabo soldado)
├── VBUS (+5V) ──────────────→ IP5306: USB_IN+
├── GND ─────────────────────→ IP5306: GND
├── CC1 ─────┬──→ R 5.1kΩ ──→ GND
└── CC2 ─────┴──→ R 5.1kΩ ──→ GND

IP5306 Module
├── BAT+ ────────────────────→ Bateria 18650 (+)
├── BAT- ────────────────────→ Bateria 18650 (-) / GND
├── SYS_OUT (+5V) ───────────→ HX711 VCC
├── SYS_OUT (+5V) ──→ chave liga-desliga ──→ ESP32 VIN/5V
├── GND ─────────────────────→ GND comum
└── LED indicators ──────────→ Status de carga (4 LEDs)

Bateria 18650 (1S)
├── (+) ─────────────────────→ IP5306: BAT+
└── (-) ─────────────────────→ IP5306: BAT- / GND comum
```

**Detalhes do IP5306:**
- **Input:** USB Type-C 5V (ou micro-USB, dependendo do módulo)
- **Battery:** 1S Li-ion (3.7V nominal, 4.2V full charge)
- **Output:** 5V boost, até 2.4A
- **Carga:** Até 2A, com proteção contra sobrecarga e descarga profunda
- **LEDs:** 4 LEDs indicam nível da bateria (25%, 50%, 75%, 100%) — acendem acima de ~3.36 / 3.57 / 3.65 / 3.91 V
- **Proteção de descarga:** o IP5306 desliga a saída 5V quando a célula cai para ~**2,8–2,9 V** (proteção interna do chip). Na prática, com queda de tensão nos fios, o corte pode ocorrer um pouco antes (~3,0–3,2 V medidos na célula)
- **Recarga automática:** reinicia carga quando a bateria cai abaixo de ~4,1 V
- **Botão:** Pressione para verificar nível ou ligar/desligar a saída

**Resistores CC (5.1kΩ) — Obrigatório:**
Carregadores modernos com saída USB-C (como carregadores de celular) usam o protocolo **USB Power Delivery (PD)**. Eles só liberam os 5V na saída se detectarem dois resistores de 5.1kΩ nos pinos CC1 e CC2 do conector, aterrados ao GND. **Sem esses resistores, o carregador não fornece energia.**

- Use resistores de **5.1kΩ com tolerância de 1%** (ideal) ou 5%
- Um resistor entre **CC1 e GND**
- Outro resistor entre **CC2 e GND**
- Dica: compre uma plaquinha "USB-C Breakout" — já vem com os resistores soldados

**Fluxo de alimentação:**
```
USB-C → IP5306 (carrega bateria) → SYS_OUT 5V ──→ HX711 VCC
                                  └─→ chave ──→ ESP32 VIN/5V (regulador 3.3V)
                                  └─→ Bateria 18650 (backup)
```

**Notas importantes:**
- O ESP32 DevKit V4 tem regulador 3.3V interno — alimente pelo pino `VIN` (5V) ou `5V`
- O HX711 pode ser alimentado pelo mesmo 5V do IP5306
- A chave deve ficar em série entre `SYS_OUT/+5V` do IP5306 e `VIN/5V` do ESP32; não interrompa o GND.
- Com a chave desligada, o ESP32 e as OLEDs ficam sem alimentação, enquanto o HX711 pode continuar ligado pelo ramo próprio de 5V.
- Todos os GNDs devem estar interligados (ESP32, HX711, IP5306, bateria, USB-C)
- **Resistores CC (5.1kΩ) são obrigatórios** para carregadores USB-C modernos funcionarem
- Para o conector USB Type-C: soldar os fios diretamente nos pads do módulo IP5306 e posicionar o conector em local acessível no enclosure

**Autonomia estimada:**
| Bateria | Capacidade | Autonomia (estimada) |
|---------|------------|---------------------|
| 18650 | 2200mAh | ~8-12 horas |
| 18650 | 3400mAh | ~12-18 horas |

*Consumo típico: ESP32 + WiFi + duas OLEDs ≈ 100-140mA*

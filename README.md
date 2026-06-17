# Balanca DIY

Balança digital baseada em ESP32 DevKit V4 com célula de carga HX711, display OLED 0.96" e interface web.

## Funcionalidades

- **Pesagem em tempo real** com filtro de média móvel
- **Tara** com barra de progresso no OLED e na web
- **Histórico de pesos** com auto-save configurável
- **Calibração** via web ou botão físico
- **Display OLED 0.96"** com 3 páginas (peso, histórico, tara)
- **Interface web** com WebSocket em tempo real (2 abas: Uso e Configurações)
- **WiFi dual mode**: STA (conecta a rede existente) + AP (ponto de acesso próprio)
- **Dois botões** com ações de curto/longo/duplo clique
- **OTA Update** via web com página customizada elegante
- **Alimentação por bateria Li-ion 1S** com módulo IP5306 e USB Type-C

## Hardware

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
| Resistores CC | 5.1kΩ 1% (obrigatório para USB-C PD) | 2 |
| Resistores Divisor | 100kΩ 1% (monitoramento de bateria) | 2 |

### Pinagem

| GPIO | Função | Detalhe |
|------|--------|---------|
| 23 | HX711 DOUT | Dados do ADC (input) |
| 22 | HX711 SCK | Clock do ADC (output) |
| 21 | OLED SDA | I2C data |
| 19 | OLED SCL | I2C clock |
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
OLED Module (4 pinos)
├── VCC ───────→ 3.3V (ESP32)
├── GND ───────→ GND
├── SDA ───────→ GPIO 21 (ESP32)
└── SCL ───────→ GPIO 19 (ESP32)
```

**Notas:**
- Endereço I2C padrão: `0x3C`
- O OLED consome ~20mA — alimente pelo 3.3V do ESP32
- Se o display não for detectado, verifique com um scanner I2C

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
├── SYS_OUT (+5V) ───────────→ ESP32 VIN (regula para 3.3V interno)
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
- **LEDs:** 4 LEDs indicam nível da bateria (25%, 50%, 75%, 100%)
- **Botão:** Pressione para verificar nível ou ligar/desligar a saída

**Resistores CC (5.1kΩ) — Obrigatório:**
Carregadores modernos com saída USB-C (como carregadores de celular) usam o protocolo **USB Power Delivery (PD)**. Eles só liberam os 5V na saída se detectarem dois resistores de 5.1kΩ nos pinos CC1 e CC2 do conector, aterrados ao GND. **Sem esses resistores, o carregador não fornece energia.**

- Use resistores de **5.1kΩ com tolerância de 1%** (ideal) ou 5%
- Um resistor entre **CC1 e GND**
- Outro resistor entre **CC2 e GND**
- Dica: compre uma plaquinha "USB-C Breakout" — já vem com os resistores soldados

**Fluxo de alimentação:**
```
USB-C → IP5306 (carrega bateria) → 5V boost → ESP32 VIN (3.3V regulator) → HX711 VCC
                                    ↓
                              Bateria 18650 (backup)
```

**Notas importantes:**
- O ESP32 DevKit V4 tem regulador 3.3V interno — alimente pelo pino `VIN` (5V) ou `5V`
- O HX711 pode ser alimentado pelo mesmo 5V do IP5306
- Todos os GNDs devem estar interligados (ESP32, HX711, IP5306, bateria, USB-C)
- **Resistores CC (5.1kΩ) são obrigatórios** para carregadores USB-C modernos funcionarem
- Para o conector USB Type-C: soldar os fios diretamente nos pads do módulo IP5306 e posicionar o conector em local acessível no enclosure

**Autonomia estimada:**
| Bateria | Capacidade | Autonomia (estimada) |
|---------|------------|---------------------|
| 18650 | 2200mAh | ~8-12 horas |
| 18650 | 3400mAh | ~12-18 horas |

*Consumo típico: ESP32 + WiFi + OLED ≈ 80-120mA*

### Diagrama Geral

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
│                 │──→ 5V ──→ ESP32 VIN               │
│  BAT+ ── 18650  │──→ Divisor (100k+100k) ──→ GPIO 34│
│  BAT- ── GND    │                                   │
└─────────────────┘

┌─────────────────────────────────────────────────────┐
│                  ESP32 DevKit V4                     │
│                                                      │
│  GPIO 23 ←── HX711 DT                               │
│  GPIO 22 ←→ HX711 SCK                               │
│  GPIO 21 ←→ OLED SDA                                │
│  GPIO 19 ←→ OLED SCL                                │
│  GPIO 34 ←── Divisor de Tensão (Bateria)            │
│  GPIO 4  ←── Botão A (GND)                          │
│  GPIO 15 ←── Botão B (GND)                          │
│                                                      │
│  WiFi: STA + AP                                     │
└─────────────────────────────────────────────────────┘
```

## PlatformIO Configuration

### platformio.ini

```ini
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino
monitor_speed = 115200
upload_speed = 921600
board_build.filesystem = littlefs
monitor_filters = esp32_exception_decoder
lib_deps =
    bogde/HX711@^0.7.5
    olikraus/U8g2@^2.36.5
    bblanchon/ArduinoJson @ ^7.3.0
    links2004/WebSockets @ ^2.6.1
build_flags =
    -DCORE_DEBUG_LEVEL=0
```

### Notas Importantes

1. **Partição de Flash:** A configuração usa a partição padrão de **4MB** (padrão do `esp32dev`). **NÃO** use `default_8MB.csv` — isso causa boot loop em chips de 4MB (ESP32-WROOM-32 genéricos).

2. **Board:** `esp32dev` é compatível com ESP32 DevKit V1, V4 e módulos ESP32-WROOM-32 de 4MB.

3. **Filesystem:** `board_build.filesystem = littlefs` habilita o upload de arquivos web via `pio run -t uploadfs`.

4. **Monitor filter:** `esp32_exception_decoder` decodifica stack traces em caso de crash.

## Bibliotecas

| Biblioteca | Versão | Uso |
|------------|--------|-----|
| **HX711** (bogde) | ^0.7.5 | Driver do ADC da célula de carga. Fornece `read()`, `read_average()`, `set_scale()`, `set_offset()`, `tare()` |
| **U8g2** (olikraus) | ^2.36.5 | Driver universal de displays. Suporta SSD1306 via I2C. Fornece `firstPage()`, `nextPage()`, `drawUTF8()`, `setFont()` |
| **ArduinoJson** (bblanchon) | ^7.3.0 | Parsing e geração de JSON para telemetria WebSocket. Usa `JsonDocument`, `JsonArray`, `JsonObject` |
| **WebSockets** (links2004) | ^2.6.1 | Servidor WebSocket síncrono. Compatível com ESP32 sem causar brownout. Fornece `WebSocketsServer`, `broadcastTXT()`, `onEvent()` |

### Por que não AsyncWebServer?

As bibliotecas `ESPAsyncWebServer` + `AsyncTCP` causam **boot loop** (`rst:0x3 SW_RESET`) no ESP32 clássico (WROOM-32) devido ao brownout detector embutido no core Arduino pré-compilado. O ESP32-S3 não tem esse problema.

**Solução adotada:** `WebServer` (nativo do ESP32) + `WebSockets` (links2004) — estável e sem brownout.

## Botões

### Botão A (GPIO 4) - Uso Geral

| Ação | Função |
|------|--------|
| Clique curto | Tara (zerar balança) |
| Clique longo (>3s) | Resetar calibração para padrões |
| Duplo clique | Ligar/desligar auto-save do histórico |

### Botão B (GPIO 15) - Funções Extras

| Ação | Função |
|------|--------|
| Clique curto | Ciclar páginas do OLED (peso → histórico → tara) |
| Clique longo (>3s) | Conectar/desconectar WiFi STA |
| Duplo clique | Exibir endereço IP no OLED |

## Display OLED

| Página | Conteúdo |
|--------|----------|
| Peso | Peso atual em kg, status da tara, auto-save |
| Histórico | Últimos 5 pesos registrados |
| Tara | Barra de progresso durante taragem |

## WiFi

O ESP32 opera em modo **AP+STA** simultaneamente:

- **AP (padrão)**: SSID `BalancaDIY`, senha `12345678`, IP `192.168.4.1`
- **STA**: conecta à rede configurada em `config.h`

Se o STA falhar, o AP permanece ativo. Acesse a interface web pelo IP exibido no OLED ou no serial monitor.

## Interface Web

### Aba: Funções de Uso

- Peso atual em destaque
- Botões: Tara, Limpar Histórico, Auto-Save
- Tabela de histórico de pesos
- Cards informativos (fator, offset, WiFi)

### Aba: Configurações

- **Calibração**: informar peso conhecido e calibrar
- **Fator Manual**: definir fator de calibração diretamente
- **Rede WiFi**: informações de STA e AP
- **Ações**: resetar calibração, OTA Update

### OTA Update

Acesse `http://<ip>/update` para fazer upload de:
- **Firmware** (`.bin`): gerado por `pio run`
- **Filesystem** (`.bin`): gerado por `pio run -t buildfs`

## Instalação

### Requisitos

- [PlatformIO](https://platformio.org/) (CLI ou VSCode extension)
- ESP32 DevKit V4 (4MB Flash)
- PlatformIO Core: `pip install platformio`

### Compilar e Upload

```bash
# Compilar firmware
pio run

# Upload do firmware
pio run -t upload

# Upload do filesystem (web files)
pio run -t uploadfs

# Monitor serial
pio device monitor --baud 115200
```

### OTA Update

Após o primeiro upload via USB, atualize pelo browser em `http://<ip>/update`

## Configuração

Edite `include/config.h` para ajustar:

```c
// WiFi STA
#define WIFI_STA_SSID       "SpiderNet2"
#define WIFI_STA_PASSWORD   "SUA_SENHA_AQUI"
#define WIFI_STA_TIMEOUT    25000

// WiFi AP
#define WIFI_AP_SSID        "BalancaDIY"
#define WIFI_AP_PASSWORD    "12345678"

// Calibração padrão
#define HX711_DEFAULT_FACTOR  420.0f

// Pins
#define HX711_DOUT  23
#define HX711_SCK   22
#define OLED_SDA    21
#define OLED_SCL    19
#define BUTTON_A    4
#define BUTTON_B    15
```

## Calibração

1. Ligue a balança sem peso
2. Pressione botão A (curto) para tarar
3. Coloque um peso conhecido na balança
4. Acesse a web → Configurações → Calibração
5. Informe o peso conhecido e clique em "Calibrar"

Ou via WebSocket: envie `calibrate:<peso>` (ex: `calibrate:1.500`)

## WebSocket API

Conecte em `ws://<ip>:81/` e envie:

| Comando | Ação |
|---------|------|
| `tare` | Executa tara |
| `calibrate:<peso>` | Calibra com peso conhecido |
| `factor:<valor>` | Define fator manualmente |
| `reset` | Reset calibração |
| `toggle_autosave` | Liga/desliga auto-save |
| `clear_history` | Limpa histórico |

Resposta (JSON):

```json
{
  "weight": 1.234,
  "tareInProgress": false,
  "tareSamples": 0,
  "tareTotal": 30,
  "factor": 420.00,
  "offset": 123456,
  "autoSave": true,
  "historyCount": 10,
  "wifiConnected": true,
  "apActive": true,
  "staIP": "192.168.1.100",
  "apIP": "192.168.4.1",
  "history": [
    {"weight": 1.20, "time": 12345},
    {"weight": 1.25, "time": 17345}
  ]
}
```

## Estrutura do Projeto

```
balanca_diy/
├── platformio.ini          # Configuração do PlatformIO
├── include/
│   └── config.h            # Constantes e configurações
├── src/
│   └── main.cpp            # Firmware principal
├── data/
│   ├── index.html          # Interface web principal
│   ├── update.html         # Página OTA customizada
│   ├── style.css           # Estilos CSS
│   └── app.js              # JavaScript (WebSocket)
└── README.md
```

## Troubleshooting

### Boot loop (rst:0x3 SW_RESET)

Causa mais comum: partição de 8MB configurada em chip de 4MB. Verifique no `platformio.ini`:
- **Remova** `board_build.partitions = default_8MB.csv`
- Use apenas `board = esp32dev` (padrão 4MB)

### WiFi STA não conecta

1. Verifique se a rede é **2.4GHz** (ESP32 não suporta 5GHz)
2. Segurança deve ser **WPA2-PSK** (não WPA3)
3. Canal do router: **1 a 11**
4. SSID e senha corretos no `config.h`
5. Veja o scan no serial monitor para confirmar que a rede é visível

### LittleFS: arquivos não encontrados

1. Rode `pio run -t uploadfs` para subir os arquivos web
2. Verifique no serial monitor se `LittleFS.begin()` retorna `true`
3. A listagem de arquivos no boot mostra quais arquivos existem

### OLED não detectado

1. Verifique a conexão I2C (SDA=21, SCL=19)
2. Endereço I2C: `0x3C` (pode ser `0x3D` em alguns módulos)
3. Alimentação: 3.3V (não 5V)

### Não carrega via USB-C

1. **Resistores CC (5.1kΩ) estão soldados?** Sem eles, carregadores USB-C modernos não fornecem energia
2. Verifique a polaridade da bateria (BAT+ e BAT- no IP5306)
3. Pressione o botão do módulo IP5306 para ativar a saída
4. Teste com outro cabo USB-C

## Licença

MIT

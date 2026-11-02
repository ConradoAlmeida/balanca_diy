# Firmware

[← Voltar ao README](../README.md) · [Hardware](HARDWARE.md) · [Instalação](INSTALACAO.md) · [Interface web](WEB.md)

> PlatformIO, bibliotecas, botões, WiFi, configuração, calibração e estrutura do código.

---

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
| Clique curto | Registrar a medição estável no histórico |
| Pressão longa (>1s) | Executar tara |

### Botão B (GPIO 15) - Navegação da OLED

| Ação | Função |
|------|--------|
| Clique curto | Ciclar peso, histórico e bateria ao soltar |
| Pressão longa (>2s) | Limpar todo o histórico |

## WiFi

O ESP32 opera em modo **AP+STA** simultaneamente:

- **AP (padrão)**: SSID `BalancaDIY`, senha `12345678`, IP `192.168.4.1`
- **STA**: conecta à rede configurada em `config.h`

Se o STA falhar, o AP permanece ativo. Acesse a interface web pelo IP exibido no OLED ou no serial monitor.

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
#define HX711_TARE_SAMPLES    30
#define HX711_MOVING_AVG      15
#define WEIGHT_DISPLAY_DEADBAND_G 1.0f
#define WEIGHT_ZERO_DEADBAND_G 2.0f

// Pins
#define HX711_DOUT  23
#define HX711_SCK   22
#define OLED_SDA    21
#define OLED_SCL    19
#define OLED_ADDR   0x3C
#define BATTERY_LOW_VOLT 3.3f
#define BUTTON_A    4
#define BUTTON_B    15
```

## Calibração

1. Ligue a balança sem peso
2. Mantenha o botão A pressionado por mais de 3 segundos para tarar
3. Coloque um peso conhecido na balança
4. Acesse a web → Configurações → Calibração
5. Informe o peso conhecido em kg e clique em "Calibrar"

Ou via WebSocket: envie `calibrate:<peso>` (ex: `calibrate:1.500`)

O fator do HX711 é calculado como `contagens brutas / peso conhecido`. Sem tara e calibração, a leitura não representa um peso confiável.
O peso exibido usa média móvel de `15` amostras (`HX711_MOVING_AVG`), uma banda de estabilidade de `1 g` (`WEIGHT_DISPLAY_DEADBAND_G`) e zona morta de zero entre `-2 g` e `+2 g` (`WEIGHT_ZERO_DEADBAND_G`). Variações menores que 1 g não alteram o painel; a média acrescenta aproximadamente 1,5 segundo de resposta a 10 SPS.
A tara coleta `30` amostras e normalmente inicia/conclui em aproximadamente 3 segundos.

Se a leitura continuar variando vários gramas com a mesma massa, verifique se a célula está rigidamente fixada, se a plataforma não encosta na estrutura, se nenhum cabo força a célula e se o HX711 recebe 5V com GND comum. A média não corrige problemas mecânicos ou elétricos.

### Diagnóstico rápido

- No boot, o monitor serial deve mostrar `OLED: OK`.
- A OLED deve estar em SDA=21/SCL=19.
- Se a tela apagar após um período sem interação, confirme `OLED_SCREEN_TIMEOUT`; o padrão do protótipo é `0` (sem desligamento automático).

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

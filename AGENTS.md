# AGENTS.md — Balanca DIY

Instruções para agentes de IA e automação que trabalharem neste repositório.

## O que é

Firmware ESP32 (PlatformIO/Arduino) de uma balança com célula de carga HX711,
OLED SSD1306 e interface web via WebSocket. O mesmo firmware roda em ESP32
DevKit V1/V4 com 4 MB de flash.

O `README.md` é um índice. Leia o documento da área em `docs/` conforme a
tarefa: `HARDWARE.md`, `BATERIA.md`, `FIRMWARE.md`, `INSTALACAO.md`, `WEB.md` ou
`TROUBLESHOOTING.md`. Não carregue o README inteiro.

## Comandos

```bash
pio run                         # compilar
pio run -t upload               # gravar firmware
pio run -t uploadfs             # gravar data/ no LittleFS
pio device monitor --baud 115200
```

## Regras que não podem ser quebradas

1. **Nunca comite credenciais reais de WiFi.** `include/config.h` é versionado;
   deixe SSID/senha como placeholder. Segredos ficam fora do Git.
2. **Partição de 4 MB.** Não use `board_build.partitions = default_8MB.csv` em
   ESP32-WROOM-32 de 4 MB: causa boot loop.
3. **Não troque para `ESPAsyncWebServer`/`AsyncTCP`.** No ESP32 clássico isso
   causa brownout/boot loop; o projeto usa `WebServer` + `WebSockets`.
4. **Arquivos web ficam em `data/`** e só vão para a placa com
   `pio run -t uploadfs`.
5. **Pinagem fixa:** HX711 em GPIO 23/22, OLED em 21/19, bateria em 34, botões
   em 4 e 15. Mudou? Atualize `docs/HARDWARE.md` e `README.md`.
6. **Calibração e tara** dependem de `HX711_DEFAULT_FACTOR`, amostras e deadbands
   em `config.h`; não altere sem atualizar `docs/FIRMWARE.md`.

## Mapa rápido

- `src/main.cpp` — firmware: HX711, OLED, botões, WiFi, WebSocket e OTA.
- `include/config.h` — WiFi, pinos, calibração e constantes.
- `data/` — `index.html`, `update.html`, `style.css`, `app.js` (LittleFS).
- `platformio.ini` — board, framework, libs e filesystem.
- `docs/` — manuais por área.

## Verificação

Não há testes automatizados. Antes de concluir:

1. `pio run` compila sem erro.
2. Se mexeu em `data/`, rode `pio run -t uploadfs`.
3. No monitor serial, confirme `OLED: OK`, WiFi e leitura do HX711.

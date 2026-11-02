# Troubleshooting

[← Voltar ao README](../README.md) · [Instalação](INSTALACAO.md) · [Firmware](FIRMWARE.md)

> Sintomas e correções dos problemas mais comuns.

---

## Boot loop (rst:0x3 SW_RESET)

Causa mais comum: partição de 8MB configurada em chip de 4MB. Verifique no `platformio.ini`:
- **Remova** `board_build.partitions = default_8MB.csv`
- Use apenas `board = esp32dev` (padrão 4MB)

## WiFi STA não conecta

1. Verifique se a rede é **2.4GHz** (ESP32 não suporta 5GHz)
2. Segurança deve ser **WPA2-PSK** (não WPA3)
3. Canal do router: **1 a 11**
4. SSID e senha corretos no `config.h`
5. Veja o scan no serial monitor para confirmar que a rede é visível

## LittleFS: arquivos não encontrados

1. Rode `pio run -t uploadfs` para subir os arquivos web
2. Verifique no serial monitor se `LittleFS.begin()` retorna `true`
3. A listagem de arquivos no boot mostra quais arquivos existem

## OLED não detectado

1. Verifique a conexão I2C (SDA=21, SCL=19)
2. Verifique SDA=21 e SCL=19 no I2C hardware
3. Alimentação: 3.3V (não 5V)

## Não carrega via USB-C

1. **Resistores CC (5.1kΩ) estão soldados?** Sem eles, carregadores USB-C modernos não fornecem energia
2. Verifique a polaridade da bateria (BAT+ e BAT- no IP5306)
3. Pressione o botão do módulo IP5306 para ativar a saída
4. Teste com outro cabo USB-C

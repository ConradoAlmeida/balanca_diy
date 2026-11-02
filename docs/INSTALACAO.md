# Instalação

[← Voltar ao README](../README.md) · [Firmware](FIRMWARE.md) · [Interface web](WEB.md)

> Requisitos, compilação, upload e OTA.

---

## Build e upload

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

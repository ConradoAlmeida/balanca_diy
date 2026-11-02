# Balanca DIY

Balança digital baseada em ESP32 DevKit V4 com célula de carga HX711, tela OLED
0.96" e interface web. Mede, tara, registra histórico, calibra pelo navegador e
atualiza por OTA.

## Funcionalidades

- **Pesagem em tempo real** com filtro de média móvel
- **Tara** por pressão longa no botão A, com indicação no OLED e na web
- **Tara automática ao ligar** usando a média de amostras do HX711
- **Histórico de pesos** registrado manualmente pelo clique curto do botão A
- **Calibração** via web ou botão físico
- **Tela OLED 0.96"** com páginas de peso, histórico e bateria
- **Interface web** com WebSocket em tempo real (2 abas: Uso e Configurações)
- **WiFi dual mode**: STA (conecta a rede existente) + AP (ponto de acesso próprio)
- **Dois botões** para registro, tara e navegação das páginas da OLED
- **OTA Update** via web com página customizada elegante
- **Alimentação por bateria Li-ion 1S** com módulo IP5306 e USB Type-C

## Início rápido

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

## Documentação

| Documento | Conteúdo |
|---|---|
| [Hardware](docs/HARDWARE.md) | Componentes, pinagem, wiring, OLED e diagrama |
| [Mecânica](docs/MECANICA.md) | Projeto SolidWorks no NAS: SLDPRT, STEPs e como usar |
| [Bateria](docs/BATERIA.md) | Curva de percentual, alarme e autonomia |
| [Firmware](docs/FIRMWARE.md) | PlatformIO, bibliotecas, botões, WiFi, config e calibração |
| [Instalação](docs/INSTALACAO.md) | Requisitos, build, upload e OTA |
| [Interface web e API](docs/WEB.md) | Abas, OTA e comandos WebSocket |
| [Troubleshooting](docs/TROUBLESHOOTING.md) | Sintomas e correções |
| [Regras para agentes](AGENTS.md) | Build, convenções e regras do repositório |

## Estrutura do projeto

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

## Licença

MIT

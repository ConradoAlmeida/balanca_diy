# Interface web e API

[← Voltar ao README](../README.md) · [Instalação](INSTALACAO.md) · [Firmware](FIRMWARE.md)

> Abas da interface, OTA e comandos WebSocket.

---

## Interface

### Aba: Funções de Uso

- Peso atual em destaque, exibido em gramas
- Botões: Registrar Medição (A), Tara (A longo), Próxima Página (B) e Limpar Histórico
- O histórico também é registrado pelo clique curto do botão A
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

## WebSocket API

Conecte em `ws://<ip>:81/` e envie:

| Comando | Ação |
|---------|------|
| `tare` | Executa tara |
| `record` | Registra a medição estável no histórico |
| `next_page` | Alterna entre peso, histórico e bateria |
| `calibrate:<peso>` | Calibra com peso conhecido |
| `factor:<valor>` | Define fator manualmente |
| `reset` | Reset calibração |
| `clear_history` | Limpa histórico |

Resposta (JSON):

```json
{
  "weight": 1.234,
  "tareInProgress": false,
  "tareSamples": 0,
  "tareTotal": 10,
  "factor": 420.00,
  "offset": 123456,
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

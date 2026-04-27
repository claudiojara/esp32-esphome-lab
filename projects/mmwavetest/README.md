# mmwavetest

Sensor mmWave Hi-Link **HLK-LD2410C** integrado a HA: detección de presencia, distancia de blanco móvil y blanco quieto, energía de cada blanco. Usa el componente externo `ld2410` de ESPHome.

## Hardware

- ESP32-C3 SuperMini.
- HLK-LD2410C (módulo mmWave 24GHz, 5V).

## Pinout

| Función | ESP pin | Sensor |
|---------|---------|--------|
| 5V | 5V | VCC |
| GND | GND | GND |
| UART TX (ESP→sensor) | GPIO4 | RX |
| UART RX (sensor→ESP) | GPIO3 | TX |

UART a **256000 baud** (no 115200 como otros sensores HLK — es la rate específica del LD2410C).

## Entidades expuestas a HA

- `binary_sensor.presence` — true si hay alguien en el rango configurado.
- `binary_sensor.moving_target` — true si hay movimiento.
- `binary_sensor.still_target` — true si hay alguien quieto.
- `sensor.moving_distance` — cm al blanco móvil.
- `sensor.still_distance` — cm al blanco quieto.
- `sensor.detection_distance` — distancia agregada.
- `sensor.moving_energy` / `sensor.still_energy` — intensidad de la señal (0-100).

## Calibración

Desde HA se puede ajustar:
- Sensibilidad por gate (gates = "anillos" de distancia, 0.75m cada uno).
- Distancia máxima de detección.
- Tiempo de inactividad antes de marcar "no presence".

## Build & flash

```bash
esphome config mmwavetest.yaml
esphome run mmwavetest.yaml --device /dev/cu.usbmodem<TAB>
esphome run mmwavetest.yaml          # OTA
esphome logs mmwavetest.yaml
```

## Si los datos no llegan

Si `binary_sensor.presence` queda permanente en `unknown` y no hay datos, el problema casi siempre es **cableado o cold joints en GPIO3/GPIO4**. Carga `../mmwavetest-debug/mmwavetest-debug.yaml` que muestra UART raw y diagnostica al toque (bytes con rampa RC tipo `C0:E0:F0:FC` = short entre líneas, soldadura mala).

Ver también `../../skill/` y `../../CLAUDE.md` por más debugging del sensor.

## Conflicto con `esp32-led-ring`

Este firmware usa GPIO3 para UART RX. **No coexiste** con `esp32-led-ring` (que usa GPIO3 para data del WS2812B) en el mismo ESP — son firmwares independientes para boards distintas, o se elige uno o el otro.

# esp32-test-oled-cover

ESP32-C3 + OLED 1.3" SH1106 mostrando hora actual y porcentaje de `cover.curtain` traído desde Home Assistant. Primer experimento de display I²C en este lab.

## Hardware

- ESP32-C3 SuperMini.
- OLED I²C 1.3" 128×64 SH1106 (clones chinos típicos: PCB marcado "1.30\"IIC", silkscreen "VDD GND SCK SDA").

## Pinout

| Función | ESP pin | OLED pin |
|---------|---------|----------|
| 3.3V | 3V3 | VDD |
| GND | GND | GND |
| I²C SDA | GPIO5 | SDA |
| I²C SCL | GPIO6 | SCK |
| LED builtin | GPIO8 (active-low) | — |

## Gotchas críticos

- **`model: "SH1106 128x64"`** — los clones de 1.3" son SH1106, NO SSD1306. Si pones SSD1306 no se ve nada o sale corrido.
- **`contrast: 100%` OBLIGATORIO** — sin esto el driver inicia con contrast 0 y la pantalla queda negra aunque el I²C scan la detecte. Por mucho tiempo va a parecer que el display está roto cuando no lo está.
- **Fuentes**: usa `gfonts://Roboto`. ESPHome baja las fuentes solo en el primer compile.

## Setup en HA

Requiere autorizar el device para acceder a `cover.curtain`:
1. Después del primer flasheo, HA → Settings → Devices & Services → ESPHome device → **Configure**.
2. Tildar `cover.curtain` en la lista de entidades expuestas. Sin esto el device no recibe el estado.

## Build & flash

```bash
esphome config esp32-test-oled-cover.yaml
esphome run esp32-test-oled-cover.yaml --device /dev/cu.usbmodem<TAB>
esphome run esp32-test-oled-cover.yaml          # OTA
esphome logs esp32-test-oled-cover.yaml
```

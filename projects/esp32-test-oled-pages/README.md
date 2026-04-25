# esp32-test-oled-pages

Dashboard multi-página sobre OLED SH1106. Cuatro pantallas que rotan automáticamente cada 5s o se navegan con botón físico / virtual de HA: cortina, clima (reloj + ícono + temp), sistema, avatar dithereado.

## Hardware

- ESP32-C3 SuperMini.
- OLED 1.3" SH1106 128×64 I²C.
- Botón físico push-to-GND (sin resistor externo, usa pullup interno).

## Pinout

| Función | ESP pin | Periférico |
|---------|---------|------------|
| I²C SDA | GPIO5 | OLED SDA |
| I²C SCL | GPIO6 | OLED SCK |
| Botón físico | GPIO7 | A GND, pullup interno |
| LED builtin | GPIO8 | (active-low, `inverted: true`) |

**GPIO7 elegido a propósito** — es el GPIO seguro libre. Evitar GPIO2/3/9 (strapping) y GPIO5/6 (ya en uso por I²C).

## Páginas

1. **Cortina** — % de `cover.curtain` con barra de progreso.
2. **Clima** — hora HH:MM grande + ícono PNG dinámico + temperatura.
3. **Sistema** — IP, uptime, RSSI WiFi.
4. **Avatar** — `avatar.png` dithereado a 1-bit (incluido en esta carpeta).

## Controles

- **Auto-rotate**: `switch.auto_rotate_pages` en HA. Cuando está ON rota cada 5s.
- **Next page**: `button.next_page` (virtual en HA) o el botón físico en GPIO7.

## Íconos del clima

El YAML fetchea íconos PNG desde `http://<HA-IP>:8123/local/weather/<state>.png`. Hay que subir manualmente a `/config/www/weather/` en HA los PNGs:
- `sunny.png`, `partlycloudy.png`, `cloudy.png`, `rainy.png`, `snowy.png`, `clear-night.png`, etc.
- `unknown.png` como fallback.

`substitutions.ha_base_url` en el YAML es el IP de HA — editarlo si la red cambia.

## Build & flash

```bash
esphome config esp32-test-oled-pages.yaml
esphome run esp32-test-oled-pages.yaml --device /dev/cu.usbmodem<TAB>
esphome run esp32-test-oled-pages.yaml          # OTA
esphome logs esp32-test-oled-pages.yaml
```

## Files

- `esp32-test-oled-pages.yaml` — config principal.
- `avatar.png` — imagen original 1024×1024, ESPHome la baja resolución y dithera al compilar.
- `secrets.yaml` — symlink a `../../secrets.yaml`.

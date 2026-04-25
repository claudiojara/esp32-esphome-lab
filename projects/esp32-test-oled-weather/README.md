# esp32-test-oled-weather

Versión simplificada del dashboard: una sola pantalla con reloj + clima dinámico. Sirve como base para construir un meteo widget de mesa.

## Hardware

- ESP32-C3 SuperMini.
- OLED 1.3" SH1106 128×64 I²C.

## Pinout

| Función | ESP pin | OLED |
|---------|---------|------|
| I²C SDA | GPIO5 | SDA |
| I²C SCL | GPIO6 | SCK |

## Lo que muestra

- Hora `HH:MM` grande arriba.
- Ícono PNG del clima al medio (cambia según `weather.home`).
- Temperatura abajo.

## Setup obligatorio en HA

El YAML usa `online_image` que **fetchea PNGs desde HA** sobre HTTP. Hay que subir manualmente a `/config/www/weather/` en HA:

```
sunny.png
partlycloudy.png
cloudy.png
rainy.png
pouring.png
snowy.png
clear-night.png
fog.png
windy.png
unknown.png   ← fallback obligatorio
```

PNGs cuadrados, ~64×64px en blanco/negro. `substitutions.ha_base_url` en el YAML es el IP de HA — editarlo si la red cambia (ej: `192.168.100.X`).

## Build & flash

```bash
esphome config esp32-test-oled-weather.yaml
esphome run esp32-test-oled-weather.yaml --device /dev/cu.usbmodem<TAB>
esphome run esp32-test-oled-weather.yaml          # OTA
esphome logs esp32-test-oled-weather.yaml
```

## Diferencia vs `esp32-test-oled-pages`

Este YAML es **single-page** (más simple, menos memoria). El `oled-pages` es multi-página con rotación, botón físico y avatar — más completo pero más pesado para refactorear.

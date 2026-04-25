# Projects

Cada subcarpeta es una firmware ESPHome independiente: 1 YAML + su `secrets.yaml` (symlink a `../../secrets.yaml`) + README con cableado y comandos.

## Index

| Proyecto | Para qué sirve |
|----------|----------------|
| [`esp32-test`](esp32-test/) | LED builtin como entidad `light` controlable desde HA. El "hello world" del ESP32+HA. |
| [`esp32-test-interval`](esp32-test-interval/) | Parpadea el LED builtin cada 1s. Sin `api`. Smoke test de hardware sin necesidad de HA. |
| [`esp32-test-oled-cover`](esp32-test-oled-cover/) | LED + OLED SH1106 mostrando hora y % de `cover.curtain` desde HA. |
| [`esp32-test-oled-pages`](esp32-test-oled-pages/) | Dashboard multi-página (cortina, clima, sistema, avatar) con rotación y botón físico. |
| [`esp32-test-oled-weather`](esp32-test-oled-weather/) | Reloj + clima con ícono dinámico fetcheado desde HA. |
| [`esp32-led-ring`](esp32-led-ring/) | Anillo WS2812B 16 LEDs como `light.anillo` con 11 efectos para indicar eventos. |
| [`mmwavetest`](mmwavetest/) | Sensor mmWave HLK-LD2410C: presencia, distancia móvil/quieta, energía. |
| [`mmwavetest-debug`](mmwavetest-debug/) | Variante UART raw del mmwave para diagnosticar problemas de cableado. |

## Convención

Cada proyecto:
- Tiene su propio `device_name` y entity_ids — son firmwares **independientes** (no coexisten en un mismo ESP).
- Comparte `secrets.yaml` del root vía symlink — un único origen para WiFi y OTA passwords.
- Es buildable de forma aislada: `cd projects/<nombre> && esphome run <yaml>`.

## Comandos comunes

Desde dentro de cualquier carpeta de proyecto:

```bash
esphome config <yaml>           # validar
esphome compile <yaml>          # compilar sin flashear
esphome run <yaml> --device /dev/cu.usbmodem<TAB>   # primer flasheo (USB)
esphome run <yaml>              # OTA (después del primer flasheo)
esphome logs <yaml>              # logs en vivo vía API
```

## Empezar uno nuevo

Copiar la carpeta más parecida, renombrar el YAML, regenerar `api.encryption.key` con `openssl rand -base64 32`, ajustar `device_name` / `friendly_name`, agregar entrada al index de arriba.

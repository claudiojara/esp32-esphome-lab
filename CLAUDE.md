# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Qué es

Proyecto de prueba ESP32-C3 SuperMini integrado a Home Assistant vía ESPHome. YAML declarativo — no C++. Firmware flasheado por USB desde la Mac (HA corre como VM en Proxmox, el addon de ESPHome no ve el USB).

## Hardware

- **Board**: ESP32-C3 SuperMini (`esp32-c3-devkitm-1`, variant `esp32c3`)
- **LED built-in**: GPIO8 **active-low** → `inverted: true` es obligatorio, sin eso HA muestra el estado invertido. Es el gotcha #1 de este board.

## YAMLs

- `esp32-test.yaml` — build principal: LED como entidad `light` controlable desde HA.
- `esp32-test-interval.yaml` — variante sin `api`, parpadea el LED cada 1s. Test standalone para verificar flasheo/hardware.
- `esp32-test-oled-cover.yaml` — LED + OLED 1.3" SH1106 (128×64) I²C en GPIO5 (SDA) / GPIO6 (SCL). Muestra hora + porcentaje de `cover.curtain` desde HA. Usa `gfonts://Roboto` (ESPHome baja las fuentes solo). Requiere autorizar el device en HA → Configure → tildar `cover.curtain`.
- `esp32-test-oled-pages.yaml` — Dashboard multi-página: cortina, clima (reloj+ícono+temp), sistema, avatar. Rotación cada 5s por `switch.auto_rotate_pages` + `button.next_page` (virtual HA) + **botón físico en GPIO7** (a GND, pullup interno). Fetchea íconos del clima de `http://<HA-IP>:8123/local/weather/<state>.png`. **IMPORTANTE**: con `model: "SH1106 128x64"` es obligatorio `contrast: 100%`.
- `esp32-test-oled-weather.yaml` — Reloj + tiempo con ícono dinámico. Usa `online_image` que fetchea `http://<HA-IP>:8123/local/weather/<state>.png` según cambia `weather.home`. Requiere subir PNGs a `/config/www/weather/` en HA: sunny/cloudy/rainy/etc + unknown.png fallback. `substitutions.ha_base_url` es el IP de HA — editar si cambia.

Todos comparten `wifi`, `ota`, `api` (encriptada) y `captive_portal`.

## Secretos

`secrets.yaml` está gitignoreado. Copiar de `secrets.yaml.example` y completar. Claves requeridas: `wifi_ssid`, `wifi_password`, `fallback_password`, `ota_password`. La `api.encryption.key` está hardcodeada en el YAML (regenerar con `esphome config` si se comparte).

## Comandos

ESPHome instalado vía `uv tool install esphome` (no pip/pipx).

```bash
# Validar config
esphome config esp32-test.yaml

# Compilar sin flashear
esphome compile esp32-test.yaml

# Primera vez (flashea por USB). Listar puertos: ls /dev/cu.usbmodem*
esphome run esp32-test.yaml --device /dev/cu.usbmodem<TAB>

# OTA posteriores (el device aparece en mDNS)
esphome run esp32-test.yaml

# Logs en vivo
esphome logs esp32-test.yaml
```

`.esphome/` es cache de build — gitignoreado.

## Integración con HA

Después del primer flasheo y que el ESP se conecte al WiFi, HA lo autodescubre vía mDNS. Agregar integración **ESPHome** desde Settings → Devices & Services, pegar la `api.encryption.key` del YAML.

## Skill relacionado

Hay un skill global `esp32-homeassistant` en `~/.claude/skills/esp32-homeassistant/` con el workflow completo (discovery questions, tabla de boards/LEDs, ejemplos). Cargarlo cuando se arranca un proyecto ESP32 nuevo.

## Upgrade de pantalla pendiente

La SH1106 1-bit limita visualmente el proyecto (avatar dithereado, sin color, sin espacio para gráficos). Próximo upgrade evaluado: **ST7789 IPS 1.3" 240×240** (~$5-8 USD, 7× más píxeles, color 16-bit). SPI en vez de I²C → 5-6 pines vs 2. Compatible ESPHome con platform `st7789v`. Alternativa cool: **GC9A01 1.28" redonda**. Ver topic `hardware/display-upgrade-st7789` en engram para detalles.

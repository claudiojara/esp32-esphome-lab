<div align="center">

# esp32-esphome-lab

**ESP32-C3 SuperMini + ESPHome + Home Assistant** — OLED dashboards, mmWave presence, YAML-declarative firmware.

<img src="assets/demo.gif" alt="Avatar page reacting to mmWave presence with live distance" width="480" />

[![License: MIT](https://img.shields.io/badge/license-MIT-green.svg)](LICENSE)
[![ESPHome](https://img.shields.io/badge/ESPHome-2026.4+-black?logo=esphome&logoColor=white)](https://esphome.io)
[![Home Assistant](https://img.shields.io/badge/Home_Assistant-compatible-18BCF2?logo=home-assistant&logoColor=white)](https://www.home-assistant.io)
[![Platform: ESP32-C3](https://img.shields.io/badge/Platform-ESP32--C3-E7352C?logo=espressif&logoColor=white)](https://www.espressif.com/en/products/socs/esp32-c3)
[![Language: YAML](https://img.shields.io/badge/Config-YAML-CB171E?logo=yaml&logoColor=white)](https://yaml.org)

</div>

---

## Stack Tecnológico

<table>
  <tr>
    <td align="center" width="180">
      <img src="https://esphome.io/_images/logo-text.svg" width="56" height="56" alt="ESPHome" /><br>
      <b>ESPHome</b><br>
      <sub>Firmware declarativo</sub>
    </td>
    <td align="center" width="180">
      <img src="https://cdn.simpleicons.org/homeassistant/18BCF2" width="56" height="56" alt="Home Assistant" /><br>
      <b>Home Assistant</b><br>
      <sub>API nativa + mDNS</sub>
    </td>
    <td align="center" width="180">
      <img src="https://cdn.simpleicons.org/espressif/E7352C" width="56" height="56" alt="Espressif" /><br>
      <b>ESP32-C3</b><br>
      <sub>SuperMini RISC-V</sub>
    </td>
    <td align="center" width="180">
      <img src="https://cdn.simpleicons.org/python/3776AB" width="56" height="56" alt="Python" /><br>
      <b>Python + uv</b><br>
      <sub>ESPHome CLI toolchain</sub>
    </td>
  </tr>
  <tr>
    <td align="center" width="180">
      <img src="https://api.iconify.design/mdi:monitor-screenshot.svg?color=%23ffffff" width="56" height="56" alt="OLED" /><br>
      <b>OLED SH1106</b><br>
      <sub>1.3" 128×64 I²C</sub>
    </td>
    <td align="center" width="180">
      <img src="https://api.iconify.design/mdi:motion-sensor.svg?color=%23ffffff" width="56" height="56" alt="mmWave" /><br>
      <b>HLK-LD2410C</b><br>
      <sub>24GHz mmWave UART</sub>
    </td>
    <td align="center" width="180">
      <img src="https://api.iconify.design/mdi:home-automation.svg?color=%23ffffff" width="56" height="56" alt="HAOS" /><br>
      <b>HAOS on Proxmox</b><br>
      <sub>VM sin acceso a USB</sub>
    </td>
    <td align="center" width="180">
      <img src="https://cdn.simpleicons.org/github/181717" width="56" height="56" alt="GitHub" /><br>
      <b>GitHub</b><br>
      <sub>Versioning + issues</sub>
    </td>
  </tr>
</table>

---

## Features

- **LED control desde HA** como `light` entity
- **OLED dashboards**: cortina (`cover.curtain`), clima con ícono dinámico, info de sistema, avatar embebido
- **Multi-page rotation**: 5 páginas con auto-rotate (switch en HA) + botón físico (GPIO7) + botón virtual (HA)
- **mmWave presence detection**: LD2410C por UART a 256000 baud, con distancia móvil / quieta / de detección + energía
- **Reactive UI**: overlay de presencia en todas las páginas, avatar cambia de saludo cuando detecta a alguien
- **HA online images**: iconos de clima servidos desde `/config/www/weather/<state>.png`
- **OTA nativo** post-primer-flash — ya no se vuelve a tocar el USB

---

## Quick Start

Prerequisites: Python 3.10+, [`uv`](https://github.com/astral-sh/uv).

```bash
# 1. Clonar
git clone https://github.com/claudiojara/esp32-esphome-lab.git
cd esp32-esphome-lab

# 2. Instalar ESPHome
uv tool install esphome

# 3. Copiar template de secretos y completar con tus credenciales
cp secrets.yaml.example secrets.yaml
$EDITOR secrets.yaml

# 4. Generar una API encryption key nueva por device
python3 -c "import secrets, base64; print(base64.b64encode(secrets.token_bytes(32)).decode())"
# Pegala en el api.encryption.key del YAML que vayas a flashear

# 5. Primer flash por USB (cd a la carpeta del proyecto que querés flashear)
cd projects/esp32-test
ls /dev/cu.usbmodem*
esphome run esp32-test.yaml --device /dev/cu.usbmodem<TAB>

# 6. Adoptar en HA: Settings -> Devices & Services -> ESPHome -> pegar la encryption key

# 7. A partir de ahora, OTA automático sin cable
esphome run esp32-test.yaml
```

> Cada subcarpeta de `projects/` tiene un symlink a `secrets.yaml` del root, así ESPHome encuentra los credenciales sin duplicarlos.

---

## Projects

Cada proyecto vive en su propia carpeta bajo [`projects/`](projects/) con su YAML, README y symlink a secrets. Detalle e índice completo en [`projects/README.md`](projects/README.md).

| Proyecto | Qué hace |
|----------|----------|
| [`projects/esp32-test`](projects/esp32-test/) | LED built-in como `light` HA vía GPIO8 active-low. Starting point minimal. |
| [`projects/esp32-test-interval`](projects/esp32-test-interval/) | Parpadeo standalone de 1s, sin `api`. Smoke test de hardware. |
| [`projects/esp32-test-oled-cover`](projects/esp32-test-oled-cover/) | OLED single-page mostrando `cover.curtain` position. |
| [`projects/esp32-test-oled-weather`](projects/esp32-test-oled-weather/) | OLED con reloj + ícono de clima dinámico fetchado de HA. |
| [`projects/esp32-test-oled-pages`](projects/esp32-test-oled-pages/) | **Proyecto principal.** Dashboard de 5 páginas con LD2410C integrado y UI reactiva. |
| [`projects/esp32-led-ring`](projects/esp32-led-ring/) | Anillo WS2812B 16 LEDs como `light.anillo` con 11 efectos para indicar eventos. |
| [`projects/mmwavetest`](projects/mmwavetest/) | LD2410C standalone sin display. |
| [`projects/mmwavetest-debug`](projects/mmwavetest-debug/) | Dump raw de bytes UART para diagnosticar cableado/baud. |

---

## Notas de hardware

### C3 SuperMini LED es active-low

`output.gpio.inverted: true` en GPIO8 o HA muestra el estado al revés.

### SH1106 1.3" clones necesitan `contrast: 100%` explícito

El default del driver ESPHome es 0 → pantalla completamente negra aunque I²C scan encuentre el device.

### 1.3" OLED es SH1106, NO SSD1306

Dos controladores distintos. Regla: **0.96" = SSD1306**, **1.3" = SH1106**. Driver equivocado = píxeles basura.

### LD2410C quiere 5V en VCC

Datasheet 5-12V. Con 3.3V puede bootear pero stream gibberish.

### First compile es lento

3-5 minutos la primera vez, <30s las siguientes (cache en `.esphome/`).

---

## Agent skill

La carpeta [`skill/`](skill/) contiene el espejo del agent skill global `esp32-homeassistant`. Cubre discovery → scaffolding → flashing, display integration patterns, mmWave debugging con firmas de bytes UART, y la tabla canónica de boards → LEDs → GPIOs. Ver [`skill/SKILL.md`](skill/SKILL.md).

---

## License

[MIT](LICENSE). Fork, break, learn.

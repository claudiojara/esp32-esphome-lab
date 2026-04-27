# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Convención de idioma para archivos del proyecto

Toda la documentación, README, comentarios en código, archivos de skill y cualquier otro contenido textual generado dentro de este repositorio debe escribirse en **español neutro**. No usar voseo (vos, tenés, hacelo, ponete, dale, etc.) ni regionalismos rioplatenses. Usar segunda persona del singular tú/usted o construcciones impersonales (se hace, se debe, conviene). Esta regla aplica a:

- README.md (raíz, `projects/`, cada subproyecto, `skills/`)
- Comentarios dentro de archivos `.c`, `.h`, `.yaml`, `.js`, `.py`
- Archivos de skill (`SKILL.md`, `references/`, `assets/`)
- Mensajes de commit (mantener convencional: `feat()`, `fix()`, etc.)
- Cualquier otro archivo de documentación

La conversación en chat con el usuario puede usar voseo según el output style configurado, pero los **artefactos persistentes en el repo** son siempre en español neutro.

## Qué es

Proyecto de prueba ESP32-C3 SuperMini integrado a Home Assistant vía ESPHome. YAML declarativo — no C++. Firmware flasheado por USB desde la Mac (HA corre como VM en Proxmox, el addon de ESPHome no ve el USB).

## Hardware

- **Board**: ESP32-C3 SuperMini (`esp32-c3-devkitm-1`, variant `esp32c3`)
- **LED built-in**: GPIO8 **active-low** → `inverted: true` es obligatorio, sin eso HA muestra el estado invertido. Es el gotcha #1 de este board.

## Estructura de proyectos

Cada YAML vive en su propia subcarpeta dentro de [`projects/`](projects/), con su README y un symlink a `../../secrets.yaml` para que ESPHome lo encuentre desde adentro:

```
projects/
├── README.md                    ← índice de proyectos
├── esp32-test/                  ← LED builtin como light HA (hello world)
├── esp32-test-interval/         ← parpadeo standalone, sin api (smoke test hw)
├── esp32-test-oled-cover/       ← OLED + cover.curtain%
├── esp32-test-oled-pages/       ← dashboard multi-página + mmwave + botón físico
├── esp32-test-oled-weather/     ← reloj + clima dinámico desde HA
├── esp32-led-ring/              ← anillo WS2812B 16 LEDs → light.anillo + 11 efectos
├── mmwavetest/                  ← LD2410C standalone (UART 256000 baud)
└── mmwavetest-debug/            ← dump UART raw para diagnosticar cableado
```

Detalles, cableado y gotchas por proyecto en cada README.

Todos comparten `wifi`, `ota`, `api` (encriptada) y `captive_portal`. **Cada uno es una firmware INDEPENDIENTE** — distintos `device_name`, distintos entity_ids — no coexisten en un mismo ESP físico.

## Secretos

`secrets.yaml` está gitignoreado. Copiar de `secrets.yaml.example` y completar. Claves requeridas: `wifi_ssid`, `wifi_password`, `fallback_password`, `ota_password`. La `api.encryption.key` está hardcodeada en el YAML (regenerar con `esphome config` si se comparte).

## Comandos

ESPHome instalado vía `uv tool install esphome` (no pip/pipx). **Siempre ejecutar desde dentro de la carpeta del proyecto** — el symlink a `secrets.yaml` está pensado para eso.

```bash
cd projects/esp32-test          # o el proyecto que quieras

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

Cada subcarpeta de `projects/` genera su propio `.esphome/` con su build cache — gitignoreado.

## Integración con HA

Después del primer flasheo y que el ESP se conecte al WiFi, HA lo autodescubre vía mDNS. Agregar integración **ESPHome** desde Settings → Devices & Services, pegar la `api.encryption.key` del YAML.

**Versión de HA en uso** (a 2026-04-24):

| Componente | Versión |
|-----------|---------|
| Método de instalación | Home Assistant OS |
| Core | 2026.4.3 |
| Supervisor | 2026.04.0 |
| Operating System | 17.2 |
| Frontend | 20260325.7 |

HAOS corre como VM en Proxmox. Como estamos muy por encima de 2024.8, **siempre usar `action:`** en automations / scripts / Dev Tools — `service:` está deprecated y aunque sigue funcionando por backward compat, la UI y la docs ya no lo mencionan.

## Gotchas y aprendizajes (importante leer antes de tocar)

### HA: `action:` vs `service:` (cambio en 2024.8)

Desde **Home Assistant 2024.8 (agosto 2024)** el campo `service:` en automations / scripts / Developer Tools fue renombrado a `action:`. **Ambos siguen funcionando** (backward compatible) pero `action:` es el nuevo estándar y la docs/UI lo usan exclusivamente.

```yaml
# Forma vieja (deprecated pero funciona)
service: light.turn_on
target: ...

# Forma nueva (recomendada)
action: light.turn_on
target: ...
```

Cuando ayudes al user con service calls, **siempre usar `action:`**.

### ESPHome 2026.4.x: `rmt_channel` removido del LED strip

En `esp32_rmt_led_strip` la opción `rmt_channel:` ya no existe — el driver asigna canal RMT automáticamente. YAMLs viejos que la incluyan rompen la validación con `[rmt_channel] is an invalid option`. Solución: borrar la línea.

### ESPHome: entity_id duplicado feo

ESPHome arma entity_ids como `<platform>.<device_name>_<entity_name>`. Si el `esphome.name` y el `name:` del entity son similares, queda repetido y horrible. Ejemplo real: device `esp32-led-ring` + light `LED Ring` → `light.esp32_led_ring_led_ring`.

**Solución**: usar `name: None` en el entity, así HA usa solo el `friendly_name` del device. Ejemplo en `esp32-led-ring.yaml`: device `anillo` + light `name: None` → `light.anillo`.

**Antes de pasarle al user un entity_id en automations o Dev Tools, VERIFICAR el real**: HA → Settings → Devices & Services → ESPHome device → entities. **No asumir el formato simple `light.<name>`**.

### WS2812B / NeoPixel: power y data correctos

- **NUNCA alimentar un anillo de >8 LEDs desde el USB del SuperMini**. 16 LEDs full white = ~960mA. Las trazas del C3 SuperMini se calientan y se queman. Usar fuente externa USB-C 5V/2A directo al anillo, USB del ESP a otra fuente, **GND COMÚN obligatorio**.
- **Si "le mando rojo y queda blanco"**: casi siempre es anillo SK6812 RGBW interpretado como WS2812 RGB → bytes desalineados. Cambiar `chipset: WS2812` por `chipset: SK6812` + `is_rgbw: true`.
- **Resistor 330Ω en data y cap 1000µF en VCC del anillo** son obligatorios en producción. En prototipo de mesa con cable corto suele zafar sin ellos pero el primer LED puede mostrar glitches.
- **Level shifter 74AHCT125** ideal para data (3.3V → 5V). Sin él anda en cable corto pero no es spec.
- **Diagnóstico clave**: si los logs ESPHome muestran ON/OFF llegando del HA, el data line está OK. El problema entonces es config (chipset, rgb_order) o usuario (no clickea CALL ACTION o no usa color picker).

## Skill relacionado

Hay un skill global `esp32-homeassistant` en `~/.claude/skills/esp32-homeassistant/` con el workflow completo (discovery questions, tabla de boards/LEDs, ejemplos). Cargarlo cuando se arranca un proyecto ESP32 nuevo.

## Upgrade de pantalla pendiente

La SH1106 1-bit limita visualmente el proyecto (avatar dithereado, sin color, sin espacio para gráficos). Próximo upgrade evaluado: **ST7789 IPS 1.3" 240×240** (~$5-8 USD, 7× más píxeles, color 16-bit). SPI en vez de I²C → 5-6 pines vs 2. Compatible ESPHome con platform `st7789v`. Alternativa cool: **GC9A01 1.28" redonda**. Ver topic `hardware/display-upgrade-st7789` en engram para detalles.

# esp32-led-ring

Anillo de 16 LEDs WS2812B (NeoPixel) como entidad `light.anillo` RGB addressable en HA. Pensado como **indicador de eventos**: notificaciones, alarmas, estados — color + efecto desde HA.

## Hardware

- ESP32-C3 SuperMini.
- Anillo WS2812B 16 LEDs (5V, 3 pins: VCC, GND, DIN).
- Cargador externo USB-C 5V/2A.

## Cableado

```
 ┌─ Cargador USB-C 5V/2A ──── VCC anillo
 │                       └─── GND anillo ──┐
 │                                         │
 └─ USB de la Mac ──── ESP32-C3 ───────────┤  ← GND COMÚN obligatorio
                              │            │
                              GND ─────────┘
                              GPIO3 ─────── DIN anillo
```

| Función | ESP pin | Anillo |
|---------|---------|--------|
| Data | GPIO3 | DIN |
| GND | GND | GND (también del cargador externo) |
| VCC | — | 5V cargador externo (NO del ESP) |

**El GND del cargador y del ESP DEBEN estar unidos**. Sin esto los LEDs ven el data como ruido y no responden.

## Power: regla de oro

**NO alimentar el anillo desde el USB del ESP32.** 16 LEDs full white = ~960mA — las trazas del SuperMini se calientan, brownout, y eventualmente se queman. Siempre fuente externa al anillo.

Mirá `../../CLAUDE.md` sección "Gotchas" para el deep-dive.

## Efectos disponibles

- `Pulse` / `Pulse Fast` — respiración
- `Strobe` — estroboscópico
- `Flicker` — parpadeo tipo vela
- `Rainbow` / `Rainbow Fast` — arcoíris girando
- `Color Wipe` — barrido
- `Scan` — KITT
- `Twinkle` — destellos random
- `Fireworks` — fuegos artificiales
- `Addressable Flicker` — flicker per-LED

## Build & flash

```bash
esphome config esp32-led-ring.yaml
esphome run esp32-led-ring.yaml --device /dev/cu.usbmodem<TAB>   # primer flasheo
esphome run esp32-led-ring.yaml                                  # OTA
esphome logs esp32-led-ring.yaml
```

## Uso desde HA

Entity ID: **`light.anillo`** (gracias a `name: None` en el YAML — sin esto quedaba `light.esp32_led_ring_led_ring`).

Probar desde Developer Tools → Acciones (HA 2024.8+ usa `action:`, no `service:`):

```yaml
action: light.turn_on
target:
  entity_id: light.anillo
data:
  rgb_color: [255, 0, 0]
  brightness: 200
  effect: Rainbow
```

## Ejemplos de automation

```yaml
# Timbre suena → rojo pulsando 5s
- alias: "Notif timbre"
  trigger:
    - platform: state
      entity_id: binary_sensor.doorbell
      to: "on"
  action:
    - action: light.turn_on
      target:
        entity_id: light.anillo
      data:
        rgb_color: [255, 0, 0]
        effect: Pulse
        brightness: 200
    - delay: "00:00:05"
    - action: light.turn_off
      target:
        entity_id: light.anillo
```

## Troubleshooting

| Síntoma | Causa probable |
|---------|----------------|
| Anillo prende todo random/blanco fijo y no responde | GND no común entre cargador y ESP |
| Mando rojo y queda blanco | Anillo es SK6812 RGBW, no WS2812 — cambiar `chipset: SK6812` + `is_rgbw: true` |
| Mando rojo y aparece verde/azul | `rgb_order` mal — probar `RGB` o `BRG` |
| Primer LED random, los otros bien | Falta resistor 330Ω en data line (o cable muy largo) |
| ESP se reinicia solo | Brownout — fuente débil o GND flojo |

## Config técnica

- `chipset: WS2812`
- `rgb_order: GRB`
- Data: GPIO3 (RMT auto-channel — desde ESPHome 2024.x ya no hay que asignar `rmt_channel` a mano)
- 16 LEDs

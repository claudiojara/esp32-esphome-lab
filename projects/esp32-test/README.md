# esp32-test

LED builtin del ESP32-C3 SuperMini como entidad `light` controlable desde Home Assistant. Es el "hello world" del workflow ESP32 + HA + ESPHome.

## Hardware

- ESP32-C3 SuperMini (`esp32-c3-devkitm-1`, variant `esp32c3`)
- Solo placa, sin componentes externos.

## Pinout

| Función | Pin | Notas |
|---------|-----|-------|
| LED builtin | GPIO8 | **Active-low** → `inverted: true` obligatorio |

Sin `inverted: true` HA muestra el estado al revés (on = off). Es el gotcha #1 del SuperMini.

## Build & flash

```bash
esphome config esp32-test.yaml
esphome run esp32-test.yaml --device /dev/cu.usbmodem<TAB>   # primer flasheo
esphome run esp32-test.yaml                                  # OTA
esphome logs esp32-test.yaml
```

## HA

Después del flasheo, HA descubre el device por mDNS. Ir a Settings → Devices & Services → ESPHome → Configure y pegar la `api.encryption.key` que está hardcodeada en el YAML.

Entity esperada: `light.esp32_test_led_built_in` (revisar el real en HA UI antes de armar automations — ver gotcha de entity_id en `../../CLAUDE.md`).

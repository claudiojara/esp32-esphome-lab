# esp32-test-interval

Parpadea el LED builtin cada 1 segundo. **Sin `api:`** → no aparece en HA. Sirve como smoke test del hardware: si parpadea = ESP vivo + GPIO funcional + flasheo OK.

## Hardware

- ESP32-C3 SuperMini.

## Pinout

| Función | Pin |
|---------|-----|
| LED builtin | GPIO8 (active-low, `inverted: true`) |

## Build & flash

```bash
esphome config esp32-test-interval.yaml
esphome run esp32-test-interval.yaml --device /dev/cu.usbmodem<TAB>
```

## Cuándo usarlo

- Después de comprar un ESP32-C3 SuperMini nuevo, para verificar que el board funciona.
- Si HA no descubre el device, carga este firmware como sanity check para descartar problema de hardware.
- Para validar que `inverted: true` está bien (el LED se ve parpadeando de forma regular, no apagado fijo).

No tiene OTA porque no tiene `api:` — siempre se flashea por USB.

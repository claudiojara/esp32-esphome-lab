# tetris

Tetris jugable sobre 2 paneles WS2812B 8x8 chainados verticalmente (board lógico 8 ancho × 16 alto = 128 LEDs) controlado desde Home Assistant.

**Estado actual: Phase 1 — Calibración.** Antes de escribir el engine necesitamos mapear cómo está cableado el serpentine real de los paneles. Una vez identificado, Phase 2 agrega el engine de juego completo.

## Hardware

- ESP32-C3 SuperMini
- 2 × panel WS2812B 8×8 (64 LEDs cada uno) chainados — DOUT del primero al DIN del segundo
- Fuente externa USB-C 5V/2A+ alimentando el primer panel
- USB del ESP a otra fuente (Mac u otro cargador)
- **GND común OBLIGATORIO** entre fuente del panel y ESP

### Cableado

| ESP32-C3 | Panel 1 (DIN) | Panel 1 (DOUT) | Panel 2 (DIN) | Fuente externa |
|----------|---------------|----------------|---------------|----------------|
| GPIO3    | DIN           |                |               |                |
| GND      | GND           |                | GND           | GND            |
|          | VCC           |                | VCC           | 5V             |
|          |               | DOUT →         | DIN           |                |

Resistor 330Ω en serie en data (entre GPIO3 y DIN del primer panel) y cap 1000µF en VCC del primer panel — recomendado pero zafable en mesa con cable corto.

## Phase 1 — Calibración

El YAML expone 4 modos vía el selector de efectos del `light.tetris`:

1. **Walk** — recorre LEDs 0→127 a 300ms por paso. Mirá el panel físico, anotá el orden. Logs en consola muestran el índice activo.
2. **Manual** — slider `number.manual_led_index` (0-127) en HA. Arrastrás y se prende ese LED. Útil para confirmar puntos clave (¿es el LED 0 arriba-izquierda? ¿el 64 es el primer LED del panel 2?).
3. **Stripe x8** — cada grupo de 8 LEDs en un color distinto. Revela visualmente el patrón serpentine.
4. **Halves** — primeros 64 LEDs azul, siguientes 64 rojo. Confirma que el chain entre paneles está en orden.

### Qué necesitamos saber

Después de probar los modos, queremos responder:

- ¿Dónde está físicamente el LED 0? (esquina top-left del panel de arriba, asumiendo orientación que decidamos)
- ¿Cómo serpentea cada panel? (filas alternando dirección, o todas en el mismo sentido)
- ¿La cadena entre paneles continúa el patrón o "salta"?

Con eso definimos la función `physical_index(x, y) → int` que el engine usará para renderizar el board lógico 8×16 al strip de 128 LEDs.

## Comandos

```bash
cd projects/tetris

# Validar
esphome config tetris.yaml

# Primera vez por USB
esphome run tetris.yaml --device /dev/cu.usbmodem<TAB>

# OTA después
esphome run tetris.yaml

# Logs en vivo (útil para Walk mode)
esphome logs tetris.yaml
```

## Phase 2 — pendiente

Engine de Tetris en `tetris_engine.h` (custom header), expuesto vía:

- `button.tetris_left` / `tetris_right` / `tetris_rotate` / `tetris_drop` / `tetris_new_game`
- `sensor.tetris_score` / `tetris_lines`

Tick fijo, 7 tetrominoes, line clear, sin SRS kicks ni hold ni preview en v1.

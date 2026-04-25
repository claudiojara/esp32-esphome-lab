# matrix8x8

LED matrix 8×8 con driver MAX7219 controlable desde HA: muestra texto que viene de un `input_text` de HA, scroll automático, switch on/off.

**Phase 1**: hardware test + HA visible. Sin juego todavía.
**Phase 2 (futuro)**: Snake / Tetris simplificado en lambdas ESPHome o en PlatformIO + MQTT.

## Hardware

- ESP32-C3 SuperMini (board nuevo, distinto del led-ring y los OLED).
- Módulo MAX7219 8×8 LED matrix (chip MAX7219CNG sobre PCB con header de 5 pines).

## Pinout

```
ESP32-C3 SuperMini       MAX7219 8x8
─────────────────        ──────────
3V3                ───── VCC
GND                ───── GND
GPIO5  (MOSI)      ───── DIN
GPIO4  (CS)        ───── CS
GPIO6  (CLK)       ───── CLK
```

| Función | ESP pin | Módulo |
|---------|---------|--------|
| 3.3V | 3V3 | VCC |
| GND | GND | GND |
| SPI MOSI / Data In | GPIO5 | DIN |
| SPI Chip Select | GPIO4 | CS |
| SPI Clock | GPIO6 | CLK |

### ¿Por qué 3.3V y no 5V?

El datasheet del MAX7219 dice 4-5.5V. Pero a 5V VCC, el threshold de logic high es 0.7×5 = **3.5V** — y el ESP32-C3 saca solo **3.3V**. Comunicación marginal (puede andar o no, depende del lote del chip).

Con **VCC = 3.3V**, el threshold baja a 0.7×3.3 = 2.31V → ESP32 a 3.3V cumple sobrado. Trade: brillo un toque más bajo. Para POC y matriz dentro de cuarto es perfecto.

Si querés brillo full → fuente externa 5V + level shifter 74AHCT125 en DIN/CS/CLK.

## Setup en HA (obligatorio antes de flashear)

Necesitamos un **helper de tipo Text** en HA que el ESP suscriba para sacar el mensaje a mostrar:

1. HA → **Settings → Devices & Services → Helpers → ⊕ Create Helper**.
2. Elegir **Text**.
3. Nombre: `Matrix Message`.
4. Min length: 0, Max length: 100.
5. Confirmar — queda como `input_text.matrix_message`.

Después podés cambiar el texto desde:
- HA Dashboard (agregás un card de tipo `entity` apuntando al helper).
- Developer Tools → States → editar el state de `input_text.matrix_message`.
- Cualquier automation con `action: input_text.set_value`.

## Build & flash

```bash
cd projects/matrix8x8
esphome config matrix8x8.yaml
esphome run matrix8x8.yaml --device /dev/cu.usbmodem<TAB>   # primer flasheo
esphome run matrix8x8.yaml                                  # OTA
esphome logs matrix8x8.yaml
```

## Adopción en HA

Después del primer flasheo, HA descubre el device por mDNS (`matrix8x8.local`).

1. **Settings → Devices & Services → Notifications** → "Discovered: Matrix 8x8" → **Configure**.
2. Pegar la encryption key:
   ```
   BRPr355g4GzCdYjCe4LXvEVKD4WnggTlbJtPKco38ZA=
   ```
3. Aceptar.

Entidades expuestas:
- `switch.matrix8x8` (o similar — verificar el real, recordar gotcha de entity_id).
- Texto leído de `input_text.matrix_message`.

## Test inicial

1. Switch en ON desde HA → tenés que ver scrolleando "HOLA HA" (fallback porque el helper aún está vacío).
2. Settear `input_text.matrix_message` = "Hola Claudio" → tiene que cambiar el texto en la matriz.
3. Switch en OFF → matriz queda en negro.

## Diagnóstico

| Síntoma | Causa probable |
|---------|----------------|
| Matriz sin prender ni un LED | VCC/GND mal, o SPI no inicializa — revisá pinout |
| LEDs random encendidos al boot, después no responden | DIN/CS/CLK invertidos entre sí |
| Texto pero girado/espejado | `rotate_chip:` mal — agregar `rotate_chip_180` o equivalente |
| Brillo muy bajo aunque `intensity: 5` | VCC en 3.3V — esperable. Subir a 5V con level shifter para más brillo |
| Texto cortado / ilegible | Press Start 2P size 8 puede ser muy ancho — probar `size: 6` o BDF font |

## Phase 2 — Snake / Tetris (futuro)

Cuando Phase 1 funcione, dos caminos:

### Opción A — Snake en lambdas ESPHome (más simple)
- Globals para posición de la cabeza, cuerpo (vector), comida.
- `interval: 200ms` que mueve la serpiente.
- 4 binary_sensor en GPIOs como botones de dirección.
- Display lambda dibuja el frame.
- HA expone score como sensor.

### Opción B — PlatformIO + Arduino + MQTT (para Tetris)
- Librería [`MD_MAX72XX`](https://github.com/MajicDesigns/MD_MAX72XX) (gold standard para MAX7219).
- Game loop en C++ real, mucho más limpio.
- MQTT para HA: publica `score`, `running`, suscribe `cmd/start`, `cmd/restart`.
- Requiere Mosquitto addon en HA (ya lo tenés).

Por ahora foco en Phase 1: **que se vea en HA**.

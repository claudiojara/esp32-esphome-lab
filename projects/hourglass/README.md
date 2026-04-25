# hourglass

Reloj de arena digital con 2× LED matrix MAX7219 8×8 sobre ESP32-C3 SuperMini, programado en **PlatformIO + Arduino C++** (no ESPHome).

**Sin acelerómetro** — el flip se controla via HA (MQTT) o botón físico, no por gravedad real. Si después conseguís un MPU6050 lo sumamos.

## Por qué PlatformIO y no ESPHome

| | ESPHome lambdas | PlatformIO + Arduino |
|---|-----------------|----------------------|
| Sand physics loop (128 celdas) | YAML feo, `globals:` y lambdas anidadas | C++ real, clases, arrays, debugging normal |
| MD_MAX72XX library | No disponible | Acceso directo |
| Game logic mantenible | 😬 | 😎 |
| Integración HA | Native API | MQTT (Mosquitto addon) |
| OTA | Built-in | `ArduinoOTA` library |

Para juegos / física / lógica compleja, PlatformIO es la herramienta adecuada. ESPHome lo seguís usando para sensores y displays simples (matrix8x8, anillo, OLEDs).

## Hardware

| Componente | ¿Tenés? | Notas |
|-----------|---------|-------|
| ESP32-C3 SuperMini | ✅ | Board nuevo, distinto del matrix8x8 / led-ring |
| 2× MAX7219 8×8 LED matrix | ✅ | Encadenados via DOUT→DIN |
| Estructura física a 45° | ❌ | Las matrices van rotadas en diamond, esquinas tocándose en el medio (cuello) |
| MPU6050 acelerómetro | ❌ (futuro) | Para versión "real" reactiva a gravedad. Sin esto, el flip es manual desde HA o botón |
| Botón físico (opcional) | ❌ | Para reset/restart sin abrir HA |

## Pinout planificado

```
ESP32-C3 SuperMini       MAX7219 chain (panel A → DOUT → panel B DIN)
─────────────────        ─────────────
3V3                ───── VCC (ambos paneles)
GND                ───── GND (ambos paneles)
GPIO5  (MOSI)      ───── DIN del primer panel
GPIO4  (CS)        ───── CS de ambos (en chain)
GPIO6  (CLK)       ───── CLK de ambos (en chain)

GPIO7              ───── Botón físico opcional (a GND, pullup interno)
GPIO8              ───── LED builtin (active-low) — diagnóstico
```

VCC en 3.3V por la misma razón que en `matrix8x8` — evitar logic level mismatch sin level shifter.

## Phases del proyecto

### Phase 1.5 — Blink test (ahora)
Confirmar que el toolchain PlatformIO funciona en el SuperMini.
- ✅ Compila
- ✅ Flashea por USB
- ✅ LED builtin parpadea cada 500ms
- ✅ Serial monitor muestra "blink"

### Phase 2.A — MAX7219 chain estática
- Sumar lib `MD_MAX72XX`
- Init de los 2 paneles encadenados
- Dibujar patrón estático: panel A lleno de "arena" (todos los pixels ON), panel B vacío
- Confirma: chain funciona, orden DOUT→DIN OK, brillo decente

### Phase 2.B — WiFi + MQTT
- Conectar a la red
- Cliente MQTT al broker Mosquitto del HA
- Publicar `state` topic, suscribir `cmd` topic
- HA discovery via MQTT (entidades aparecen solas en HA)

### Phase 2.C — Sand physics básico
- Game tick cada N ms
- Cada grano busca celda libre debajo (gravity vector fijo apuntando al "cuello")
- Acumulación en el corner del panel A
- Sin transición todavía entre paneles

### Phase 2.D — Neck warp
- Cuando un grano del panel A llega al corner del cuello, salta al corner opuesto del panel B
- Ahí caen y se acumulan en el panel B según gravity en panel B

### Phase 2.E — Timing
- Calcular rate de drops para que la transferencia total (64 grains) dure N minutos configurables
- Number entity en HA: 1-60 min, default 5

### Phase 2.F — Modos / polish
- Modo "ciclo continuo" (cuando termina, vuelve a empezar)
- Modo "one-shot" con notificación a HA cuando termina
- Botón físico para restart
- Animación de "flip" cuando el usuario reinicia (granos suben hacia arriba)

### Phase 3 (opcional, requiere MPU6050) — Real-time gravity
- Acelerómetro detecta giro
- Gravity vector actualiza en tiempo real
- Granos reaccionan: si das vuelta el dispositivo, suben al otro panel naturalmente

## Setup

### 1. Instalar PlatformIO Core (si no lo tenés)

```bash
# Vía pip (system Python)
pip install platformio

# O vía uv (preferido en este lab)
uv tool install platformio
```

### 2. Configurar secrets

```bash
cp include/secrets.h.example include/secrets.h
$EDITOR include/secrets.h
# completar WIFI_SSID, WIFI_PASSWORD, MQTT_HOST, MQTT_USER, MQTT_PASSWORD, OTA_PASSWORD
```

`include/secrets.h` está gitignoreado.

### 3. Compilar y flashear

```bash
cd projects/hourglass

# Listar puertos
ls /dev/cu.usbmodem*

# Build + flash
pio run -t upload --upload-port /dev/cu.usbmodem<TAB>

# Logs (Serial monitor)
pio device monitor

# O todo en uno
pio run -t upload -t monitor --upload-port /dev/cu.usbmodem<TAB>
```

Después del primer flasheo, en Phase 2.B en adelante OTA estará habilitado y no se vuelve a tocar el USB.

## Setup en HA (cuando lleguemos a Phase 2.B)

### Mosquitto MQTT broker

Si no lo tenés:
1. HA → Settings → Add-ons → Add-on Store → buscar "Mosquitto broker".
2. Install + Start.
3. Configurar usuario MQTT en HA → Settings → People → Users → ⊕ Add user (con permiso "Can only log in from the local network" desactivado para que MQTT funcione).
4. Anotar credenciales en `include/secrets.h`.

### MQTT integration en HA

Settings → Devices & Services → ⊕ Add Integration → MQTT → "Use the official add-on" → Configure.

### Discovery

El firmware publica HA discovery messages al boot. Las entidades aparecen solas en `Settings → Devices & Services → MQTT`.

## Estructura del repo

```
projects/hourglass/
├── platformio.ini             ← config del board, libs, build flags
├── src/
│   └── main.cpp               ← entrypoint (Phase 1.5 = blink)
├── include/
│   ├── secrets.h.example      ← template (commiteado)
│   └── secrets.h              ← real (gitignoreado)
├── lib/                       ← libs locales custom (ninguna por ahora)
├── README.md                  ← este archivo
└── .pio/                      ← build cache (gitignoreado)
```

## Comandos útiles

```bash
pio run                                    # solo build
pio run -t upload                          # build + flash
pio run -t clean                           # limpiar build cache
pio device monitor                         # serial monitor
pio device monitor -b 115200 --raw         # monitor con baud override
pio pkg update                             # actualizar libs
pio pkg install --library "majicdesigns/MD_MAX72XX@^3.5.0"  # ejemplo install lib
```

## Troubleshooting

| Síntoma | Causa probable |
|---------|----------------|
| `No serial port found` al flashear | ESP no en modo bootloader. Manten BOOT presionado mientras enchufás USB. En el SuperMini suele autodetectar. |
| `Serial.println` no aparece en monitor | Falta `ARDUINO_USB_CDC_ON_BOOT=1` en `build_flags` (ya está incluido en `platformio.ini`) |
| LED no parpadea pero serial sí imprime | GPIO equivocado. SuperMini = GPIO8. Otros boards = otro pin. |
| Build falla con "espressif32 platform not found" | `pio platform install espressif32` la primera vez |

## Memoria del proyecto

Logs y discoveries del development se guardan en engram bajo project `esp32-esphome-lab`, topic-prefijo `hourglass/*`.

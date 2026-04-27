# mmwavetest-debug

Variante **diagnóstica** del proyecto `mmwavetest`. En vez de parsear el protocolo del LD2410C, dumpea los **bytes raw** del UART al log. Sirve para identificar problemas físicos de cableado/soldadura.

## Cuándo usarlo

Cuando `mmwavetest` no recibe datos del sensor — antes de asumir que el sensor está roto, descarga este firmware y verifica los logs.

## Hardware

Idéntico a `mmwavetest`:

| Función | ESP pin | Sensor |
|---------|---------|--------|
| 5V | 5V | VCC |
| GND | GND | GND |
| UART RX | GPIO3 | TX del sensor |
| UART TX | GPIO4 | RX del sensor |

256000 baud, 8N1.

## Build & flash

```bash
esphome config mmwavetest-debug.yaml
esphome run mmwavetest-debug.yaml --device /dev/cu.usbmodem<TAB>
esphome logs mmwavetest-debug.yaml
```

## Cómo leer los logs

Con un sensor sano vas a ver tramas UART tipo:

```
F4:F3:F2:F1 ... 04:03:02:01
```

(la firma del LD2410C: header `F4F3F2F1`, footer `04030201`).

### Diagnósticos por patrón

| Bytes en el log | Diagnóstico |
|-----------------|-------------|
| `F4:F3:F2:F1` y datos en el medio | Sensor OK — el problema estaba en otro lado (prueba `mmwavetest`) |
| Vacío / nada | Sensor sin power, GND flojo, o cable de TX cortado |
| `C0:E0:F0:FC:FE:FF` (rampas RC progresivas) | **Short entre líneas** del UART — cold joints en la PCB del módulo o del ESP. Refluye las soldaduras |
| Bytes random sin firma | Baud rate mal o ruido eléctrico — verifica 256000 baud |
| Solo `00`s o solo `FF`s | Línea pulled en una dirección — falta resistor pullup o pin flotante |

## Caso real (sesión 2026-04-21)

Con el sensor del lab, este debug reveló bytes `C0:E0:F0:FC:FC:FE:FE:FF:FF:FF:DF` — **rampas RC** en vez de pulsos cuadrados. Foto macro reveló dos cold joints en GPIO3/GPIO4. Después de refluir, los datos empezaron a fluir correctos en `mmwavetest`.

Memoria larga del incidente en engram, topic `bugfix/mmwave-cold-joints`.

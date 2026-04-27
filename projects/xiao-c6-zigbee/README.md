# xiao-c6-zigbee

Endpoint Zigbee custom corriendo en **Seeed XIAO ESP32-C6**, joineado a un coordinador **SLZB-06** vía **Zigbee2MQTT 2.9.2 + Mosquitto** en Home Assistant.

A diferencia del resto del repo, **este proyecto NO usa ESPHome**. ESPHome no soporta Zigbee como endpoint — habla con HA por WiFi vía native API. Acá usamos `esp-zigbee-sdk` (ZBoss stack de Espressif) sobre **ESP-IDF directo**, en C.

## Stack tecnológico

[![ESP-IDF](https://img.shields.io/badge/ESP--IDF-v5.3.2-E7352C?style=for-the-badge&logo=espressif&logoColor=white)](https://docs.espressif.com/projects/esp-idf/)
[![ESP32-C6](https://img.shields.io/badge/ESP32--C6-Seeed%20XIAO-009A4E?style=for-the-badge&logo=espressif&logoColor=white)](https://wiki.seeedstudio.com/xiao_esp32c6_getting_started/)
[![C](https://img.shields.io/badge/C-A8B9CC?style=for-the-badge&logo=c&logoColor=00599C)](https://en.wikipedia.org/wiki/C_(programming_language))
[![CMake](https://img.shields.io/badge/CMake-064F8C?style=for-the-badge&logo=cmake&logoColor=white)](https://cmake.org/)
[![Zigbee](https://img.shields.io/badge/Zigbee-3.0-EB0443?style=for-the-badge&logo=zigbee&logoColor=white)](https://csa-iot.org/all-solutions/zigbee/)
[![Zigbee2MQTT](https://img.shields.io/badge/Zigbee2MQTT-2.9.2-2B3036?style=for-the-badge)](https://www.zigbee2mqtt.io/)
[![Home Assistant](https://img.shields.io/badge/Home%20Assistant-41BDF5?style=for-the-badge&logo=home-assistant&logoColor=white)](https://www.home-assistant.io/)
[![MQTT](https://img.shields.io/badge/MQTT-Mosquitto-660066?style=for-the-badge&logo=mqtt&logoColor=white)](https://mosquitto.org/)
[![Node.js](https://img.shields.io/badge/Node.js-Z2M%20converter-339933?style=for-the-badge&logo=nodedotjs&logoColor=white)](https://nodejs.org/)

| Tecnología | Rol en el proyecto | Versión |
|-----------|--------------------|---------|
| **ESP-IDF** | SDK oficial de Espressif para ESP32, build system con `idf.py` | v5.3.2 |
| **esp-zigbee-sdk** | Stack ZBoss de Espressif, expone API Zigbee 3.0 sobre 802.15.4 nativo | esp-zboss-lib + esp-zigbee-lib `~1.6.0` (managed components) |
| **C** | Lenguaje del firmware — nada de C++/Rust acá | C11 |
| **CMake** | Build system que orquesta ESP-IDF | viene con ESP-IDF toolchain |
| **Zigbee 3.0** | Protocolo wireless mesh sobre IEEE 802.15.4, profile Home Automation | — |
| **SLZB-06** | Coordinador Zigbee Smlight (Texas Instruments CC2652P) — la radio que orquesta la red | ZStack 3.x.0 firmware |
| **Zigbee2MQTT** | Bridge Zigbee ↔ MQTT, traduce paquetes Zigbee a topics MQTT que HA entiende | 2.9.2 |
| **Mosquitto** | Broker MQTT que conecta Z2M con HA | (addon HA) |
| **Home Assistant** | UI / automation engine que consume las entities expuestas por Z2M vía MQTT discovery | 2026.4.3 (HAOS 17.2) |
| **Node.js + zigbee-herdsman-converters** | Runtime donde corre el external converter `xiao-c6-light.js` con `modernExtend` | bundled con Z2M |

## Estado del proyecto

| Fase | Estado | Descripción |
|------|--------|-------------|
| 0 — Blink test | ✅ done | Toolchain ESP-IDF + LED + USB Serial/JTAG funcional |
| 1 — Zigbee on/off light | ✅ done | LED builtin como switch Zigbee, joineado a Z2M |
| 3 — External converter Z2M | ✅ done | Manuf/Model en firmware + JS converter en `zigbee2mqtt/` |
| 2 — DHT11 sensor | bloqueado | Pendiente cautín para soldar el sensor |
| 4 — Tilt sensor SW-520D | bloqueado | Idem |

## Hardware

- **Board**: Seeed XIAO ESP32-C6
- **MCU**: ESP32-C6 (RISC-V single core 160 MHz, WiFi 6, BLE 5, 802.15.4)
- **Flash**: 4 MB
- **USB**: USB-C **nativo** (no chip CP2102/CH340 — aparece directo como `/dev/cu.usbmodemXXXX` en macOS)
- **Antena**: PCB chip antenna integrada + conector u.FL para antena externa
- **LED user**: **GPIO15, active LOW**
- **Boot button**: GPIO9 (entrar a bootloader manual si auto-reset falla)

### Pinout disponible

```
        ┌──────────────┐
   D0/  │              │ /D10
   D1/  │              │ /D9
   D2/  │              │ /D8
   D3/  │              │ /D7
   D4/  │              │ /D6
   D5/  │              │ /D5
   3V3/ │              │ /BAT+
   GND/ │              │ /BAT-
        └──────USB-C───┘
```

GPIO15 (LED) y GPIO9 (BOOT) están internos al módulo, no expuestos en pads externos.

### Antena externa — recomendación

**Para el join Zigbee inicial es altamente recomendable conectar la antena u.FL.** El handshake Zigbee es multi-step (Beacon Request → Beacon → Association Request → Association Response → transferencia de Network Key → Device Announce) y cualquier paso que falle por RF débil aborta el join entero.

Una vez asociado, las network keys quedan persistidas en NVS (`zb_storage`) y la red sostiene el device aún a LQI bajo (40-65). Pero si se mueve el dispositivo o se coloca en una caja, **sin antena externa puede no sostenerse**.

## Coordinador y red Zigbee

| Parámetro | Valor |
|-----------|-------|
| Coordinador | SLZB-06 (Smlight) |
| Conexión | Ethernet TCP `tcp://192.168.100.10:6638` |
| Stack | `zstack` (CC2652P, ZStack3x0) |
| Canal Zigbee | **11** |
| Ext PAN ID | `0x1e4b2e08f3b41e34` |
| PAN ID | `10827` (`0x2a4b`) |
| TX power | 20 dBm |
| Z2M version | 2.9.2 |
| MQTT broker | Mosquitto (`mqtt://192.168.100.11:1883`) |

## Setup ESP-IDF

ESP-IDF v5.3.2 instalado en `~/esp/esp-idf`. Alias en `~/.zshrc`:

```bash
alias get_idf='. ~/esp/esp-idf/export.sh'
```

> ⚠️ **Espacio entre `.` y `~`** es obligatorio — sin él el alias no funciona porque `.` es el comando POSIX `source`, comando independiente que necesita un argumento separado.

### Por qué ESP-IDF directo y no PlatformIO

- esp-zigbee-sdk se documenta y debugea siempre con `idf.py`
- Los managed components (`espressif/esp-zboss-lib`, `espressif/esp-zigbee-lib`) se resuelven solos
- PlatformIO suele atrasar versiones de ESP-IDF
- Cuando algo rompe, se pueden copiar errores literales de issues de GitHub sin traducir

### Versiones

- ESP-IDF: `v5.3.2`
- esp-zboss-lib: `~1.6.0` (managed component)
- esp-zigbee-lib: `~1.6.0` (managed component)

## Build, flash, monitor

```bash
cd projects/xiao-c6-zigbee
get_idf

# Solo la primera vez del proyecto
idf.py set-target esp32c6

# Build (largo la primera vez por descarga de zboss + zigbee-lib, después rápido)
idf.py build

# Listar puertos USB
ls /dev/cu.usbmodem*

# Flash + monitor combinado
idf.py -p /dev/cu.usbmodem<TAB> flash monitor
```

**Salir del monitor**: `Ctrl+]`

**Reset chip sin salir del monitor**: `Ctrl+T` luego `R`

**Build + flash + retomar monitor sin salir**: `Ctrl+T` luego `F`

### Cuándo usar `erase-flash`

Solo cuando se desea borrar el join Zigbee (limpia la NVS `zb_storage`):

```bash
idf.py -p /dev/cu.usbmodem<TAB> erase-flash
```

En reflasheos normales para iterar firmware **NO usar erase-flash** — preserva el join y el device se reconecta automáticamente con `ESP_ZB_BDB_SIGNAL_DEVICE_REBOOT`.

### Modo bootloader manual (si auto-reset falla)

1. Mantener presionado **BOOT** (GPIO9)
2. Presionar y soltar **RESET**
3. Soltar **BOOT**
4. Re-correr `idf.py flash`

## Estructura del proyecto

```
xiao-c6-zigbee/
├── README.md
├── CMakeLists.txt              ← project root, llama al template ESP-IDF
├── partitions.csv              ← incluye zb_storage (16K) + zb_fct (1K)
├── sdkconfig.defaults          ← target esp32c6, ZED, native radio
├── main/
│   ├── CMakeLists.txt
│   ├── idf_component.yml       ← managed components esp-zboss-lib + esp-zigbee-lib
│   └── main.c                  ← signal handler + cluster setup + LED control
└── zigbee2mqtt/
    └── xiao-c6-light.js        ← external converter para Z2M (copiar a HA)
```

### Particiones

```csv
nvs,           data, nvs,     0x9000,   0x6000,
phy_init,      data, phy,     0xf000,   0x1000,
factory,       app,  factory, 0x10000,  0x180000,
zb_storage,    data, fat,     0x190000, 0x4000,
zb_fct,        data, fat,     0x194000, 0x1000,
```

`zb_storage` y `zb_fct` son **obligatorias** para Zigbee — guardan network key + state del join. Sin ellas el device olvida la red en cada reboot.

## External converter de Z2M

Sin el converter, Z2M autodetecta el device como genérico y la entity en HA queda con nombre feo (`switch.0x58e6c5fffe146d50`). Con el converter, matchea por `modelIdentifier` y aplica la definición:

```javascript
// projects/xiao-c6-zigbee/zigbee2mqtt/xiao-c6-light.js
const {onOff} = require('zigbee-herdsman-converters/lib/modernExtend');

module.exports = [{
    zigbeeModel: ['xiao-c6-light'],
    model: 'xiao-c6-light',
    vendor: 'ClaudioJaraLab',
    description: 'XIAO ESP32-C6 Zigbee on/off light (custom firmware)',
    extend: [onOff({powerOnBehavior: false})],
}];
```

### Instalación en Home Assistant

El archivo va en `/config/zigbee2mqtt/external_converters/` (visto desde el File Editor / Studio Code de HA queda como `homeassistant/zigbee2mqtt/external_converters/`).

**Pasos**:
1. Copiar `zigbee2mqtt/xiao-c6-light.js` → `/config/zigbee2mqtt/external_converters/xiao-c6-light.js`
2. HA → Settings → Add-ons → Zigbee2MQTT → **Restart**
3. En logs Z2M debería aparecer publicación en topic MQTT `zigbee2mqtt/bridge/converters` con el contenido del archivo (confirmación de carga OK)
4. Z2M Web UI → Dispositivos → click en el device → **Re-interview** (NO re-pair)
5. Verificar que aparece como **"soportado: external"** ✅ con manufacturer `ClaudioJaraLab` y model `xiao-c6-light`

## Match firmware ↔ converter

El match Z2M ↔ device se hace por el string `modelIdentifier` que el firmware advierte en el cluster Basic (0x0000) atributo 0x0005. El converter declara `zigbeeModel: ['xiao-c6-light']`. Cualquier mismatch (mayúsculas, guiones, espacios) rompe el match → "Not supported" en Z2M.

En el firmware (`main.c`):

```c
// ZCL strings: first byte is length, then ASCII (no null terminator)
#define MANUFACTURER_NAME    "\x0e""ClaudioJaraLab"   // 14 chars
#define MODEL_IDENTIFIER     "\x0d""xiao-c6-light"    // 13 chars
```

> ⚠️ Las strings ZCL llevan **length-prefix**, NO null-terminator. El primer byte es la cantidad de caracteres que siguen. Contar bien — un mismatch rompe todo.

Después de `esp_zb_on_off_light_ep_create()`, hay que inyectar manualmente esos atributos en el cluster Basic:

```c
esp_zb_attribute_list_t *basic_cluster = esp_zb_cluster_list_get_cluster(
    cluster_list, ESP_ZB_ZCL_CLUSTER_ID_BASIC, ESP_ZB_ZCL_CLUSTER_SERVER_ROLE);
esp_zb_basic_cluster_add_attr(basic_cluster,
    ESP_ZB_ZCL_ATTR_BASIC_MANUFACTURER_NAME_ID, MANUFACTURER_NAME);
esp_zb_basic_cluster_add_attr(basic_cluster,
    ESP_ZB_ZCL_ATTR_BASIC_MODEL_IDENTIFIER_ID, MODEL_IDENTIFIER);
```

## Conceptos Zigbee usados

| Concepto | Valor en este proyecto |
|----------|------------------------|
| Device role | End Device (ZED, batería/sleep-capable) |
| Profile | Home Automation (`ESP_ZB_AF_HA_PROFILE_ID`) |
| Endpoint ID | 10 (light) — futuros 11 (sensor temp/hum), 12 (tilt) |
| Cluster Basic (0x0000) | manuf, model, ZCL version |
| Cluster Identify (0x0003) | required para commissioning |
| Cluster On/Off (0x0006) | el state del light |
| Channel mask | `ESP_ZB_TRANSCEIVER_ALL_CHANNELS_MASK` (11-26) en dev. Optimizable a `1u << 11` cuando se confirma que joinea siempre. |
| ED aging timeout | 64 min |
| Keep-alive | 3000 ms |
| Install code policy | `false` (default trust center link key) |

## Troubleshooting

### Build / Flash

| Síntoma | Causa | Fix |
|---------|-------|-----|
| `idf.py: command not found` | env no sourceado | `get_idf` antes |
| `esp_zigbee_core.h: No such file` | managed components no bajaron | `idf.py reconfigure`, mirar `dependencies.lock` |
| No aparece `/dev/cu.usbmodem*` | Cable USB-C es solo de carga | Probar otro cable con data lines |
| Flash falla con timeout | Auto-reset no funciona | Bootloader manual (BOOT + RESET) |
| Stack overflow en boot | Task Zigbee necesita más stack | Subir `4096` → `8192` en `xTaskCreate(zb_task, ...)` |

### Pairing / Network steering

| Síntoma | Diagnóstico | Fix |
|---------|-------------|-----|
| Steering falla en <200ms (`ESP_FAIL`) | Channel mask vacío o radio no init | Usar `ESP_ZB_TRANSCEIVER_ALL_CHANNELS_MASK`, verificar `CONFIG_ZB_RADIO_NATIVE=y` |
| Steering scan completo ~2s pero falla join | Permit_join cerrado, RF débil, o canal mismatch | Confirmar permit_join en Z2M (tab logs muestra `allowing new devices to join`), conectar antena externa, acercar device al coordinador |
| Z2M no muestra el device tras join | Bug en converter o manuf/model vacío | Habilitar `log_level: debug` en Z2M, mirar logs durante el intento |
| `Stack init failed (ESP_FAIL)` durante reboot | Scheduler ZB intenta re-commission pero device ya estaba en red | **Inocuo** — eventualmente cae en `Device rebooted — already on network` |
| `In BDB commissioning, an error occurred (for example: the device has already been running)` | Idem — error informativo del scheduler | Inocuo |

### Z2M / Converter

| Síntoma | Causa | Fix |
|---------|-------|-----|
| Device aparece como "Not supported" tras pairing | Firmware no advierte manuf/model, o converter no cargó | Verificar firmware tiene `MANUFACTURER_NAME` y `MODEL_IDENTIFIER` set en cluster Basic; en Z2M chequear topic MQTT `zigbee2mqtt/bridge/converters` para ver si el archivo cargó |
| `MODULE_NOT_FOUND` al reiniciar Z2M | Path del require incorrecto en converter | Para Z2M 2.x: `require('zigbee-herdsman-converters/lib/modernExtend')` |
| Device sigue como "Not supported" tras restart Z2M | Cache vieja del device | **Re-interview** desde Z2M Web UI (NO re-pair) |
| Re-interview falla | End Device no responde a tiempo | Plan B: **remove** device en Z2M + factory reset (`erase-flash`) + permit_join + re-pair |
| Toggle desde HA no responde | Cluster handler no recibe el callback | Mirar logs del C6 cuando se hace click — debería aparecer `Attr write — ep 10, cluster 0x6, attr 0x0` |

## Aprendizajes clave

1. **ESPHome ≠ Zigbee endpoint**. Es WiFi-only. Para Zigbee custom: ESP-IDF + esp-zigbee-sdk en C.
2. **XIAO C6 antena externa para join inicial**. PCB antenna sola es marginal en hosts/closets metálicos.
3. **`erase-flash` solo cuando se desea borrar el join**. Reflasheo normal preserva NVS.
4. **Re-interview ≠ re-pair**. Cambios solo en cluster Basic → re-interview alcanza.
5. **External converters Z2M 2.x**: auto-load desde `/config/zigbee2mqtt/external_converters/`. CommonJS con `modernExtend`.
6. **Match Z2M ↔ device** por `modelIdentifier` ZCL string exacto. Cualquier diferencia rompe.
7. **ZCL strings llevan length-prefix** (`"\x0d""xiao-c6-light"`), NO null-terminator. Contar bien.
8. **Z2M topic MQTT `zigbee2mqtt/bridge/converters`** muestra los converters cargados → sirve para validar sin entrar a logs.

## Próximos pasos (cuando haya cautín)

### Phase 2 — DHT11 como segundo endpoint

- Endpoint 11 con clusters `0x0402` (Temperature Measurement) + `0x0405` (Relative Humidity)
- GPIO sugerido para DHT11: D2 (GPIO2) o D3 (GPIO3) — disponibles en pads expuestos
- Polling cada 30-60s, reportar attrs vía `esp_zb_zcl_set_attribute_val()` + reporting config
- Update converter Z2M con exposes adicionales: `e.temperature()`, `e.humidity()`

### Phase 4 — Tilt switch SW-520D como IAS Zone

- Endpoint 12 con cluster `0x0500` (IAS Zone) — el cluster Zigbee estándar para sensores de seguridad/contacto
- Reportar Zone Status como `OPEN` / `CLOSED` cuando el switch dispara
- Update converter Z2M con `e.contact()` o `e.tamper()`

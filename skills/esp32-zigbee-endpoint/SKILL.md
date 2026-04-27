---
name: esp32-zigbee-endpoint
description: >
  Build custom Zigbee endpoint firmware for ESP32-C6/H2 using esp-zigbee-sdk
  (ZBoss stack) and integrate with Home Assistant via Zigbee2MQTT external
  converters. Trigger: User mentions ESP32-C6, ESP32-H2, esp-zigbee-sdk,
  esp-zboss-lib, esp-zigbee-lib, "Zigbee endpoint custom", "Zigbee device
  custom", "zigbee esp32-c6", Zigbee2MQTT external converter, edits files
  including esp_zigbee_core.h or ha/esp_zigbee_ha_standard.h, or works with
  idf_component.yml depending on espressif/esp-zigbee-lib.
license: Apache-2.0
metadata:
  author: gentleman-programming
  version: "1.0"
---

## When to Use

- User wants to build a Zigbee endpoint device on ESP32-C6/H2 (custom sensor, light, switch, button)
- User mentions esp-zigbee-sdk, esp-zboss-lib, or wants Zigbee on ESP32 (NOT ESPHome WiFi)
- User wants to write a Zigbee2MQTT external converter for a custom device
- User asks "ESPHome vs ESP-IDF for Zigbee" or similar framework choice for Zigbee endpoints

## Critical Patterns

### ESPHome does NOT support Zigbee endpoints — use ESP-IDF

ESPHome talks to HA over WiFi via native API. It cannot expose a device as a Zigbee endpoint joined to a Z2M coordinator. For Zigbee endpoints: ESP-IDF + esp-zigbee-sdk in C, no exceptions. ESPHome experimental Zigbee component exists but is too immature for non-trivial devices.

### PlatformIO is the wrong tool for esp-zigbee-sdk

All official Espressif documentation, examples, GitHub issues, and community support assume `idf.py`. Managed components (`espressif/esp-zboss-lib`, `espressif/esp-zigbee-lib`) auto-resolve only with idf.py. PlatformIO support lags ESP-IDF versions. Save the user time: use idf.py.

### Validated toolchain combo

ESP-IDF `v5.3.2` + esp-zboss-lib `~1.6.0` + esp-zigbee-lib `~1.6.0`. For ESP-IDF 5.4+, use Zigbee libs 1.7+. ESP-IDF <5.1 lacks 802.15.4 maturity on ESP32-C6.

### Zigbee NVS partitions are mandatory

`zb_storage` (16K) and `zb_fct` (1K) partitions in `partitions.csv`. Without them the device forgets its network join on every reboot. See [assets/partitions.csv](assets/partitions.csv).

### ZCL strings are length-prefixed (NOT null-terminated)

Manufacturer name and model identifier: first byte is length (in characters), then ASCII. Example: `"\x0d""xiao-c6-light"` (0x0d = 13 chars). Mismatch of even one character breaks the Z2M converter match.

### Re-interview ≠ re-pair

When firmware changes only Basic cluster attrs (manuf/model), Z2M **Re-interview** is enough — keeps friendly_name and entities. Re-pair (factory reset + permit_join) only when device structure changes (clusters, endpoints).

### `erase-flash` semantics

Only when you want to wipe the Zigbee join (clears NVS `zb_storage`). Normal firmware iterations: do NOT use erase-flash — preserves join, device reconnects automatically via `ESP_ZB_BDB_SIGNAL_DEVICE_REBOOT`.

---

## Discovery Phase (MANDATORY — run before scaffolding)

| # | Question | Why it matters |
|---|----------|----------------|
| 1 | **Exact board?** (XIAO ESP32-C6, ESP32-C6 DevKitC-1, ESP32-H2 DevKitM-1, custom) | LED GPIO, button GPIO, USB type (native vs CP2102), antenna availability |
| 2 | **Zigbee coordinator?** (SLZB-06, Sonoff Dongle-E, SkyConnect, ConBee II) | Stack name (zstack/ember/ezsp) — only relevant if user troubleshoots Z2M |
| 3 | **Z2M version + Zigbee channel + Ext PAN ID** (Z2M Web UI → Settings → About) | Channel mask in firmware. Z2M 2.x converter format differs from 1.x |
| 4 | **Sensors/actuators on the device?** | Drives endpoint count + cluster choice (0x0006 on/off, 0x0402 temp, 0x0405 humidity, 0x0500 IAS Zone) |
| 5 | **External antenna available?** (XIAO C6 has u.FL connector) | Critical for first join. PCB antenna alone often fails the multi-step Zigbee handshake |
| 6 | **Soldering iron available?** | If no, scope is limited to LED/button builtin on the board — set expectations early |

---

## Decision Tree

| User wants... | Approach |
|---------------|----------|
| Connect ESP32 to HA without writing C | ESPHome WiFi — use `esp32-homeassistant` skill instead |
| Custom Zigbee device joining Z2M | This skill — ESP-IDF + esp-zigbee-sdk + JS converter |
| ESP32 as Zigbee coordinator (replace Sonoff stick) | Out of scope — flash Espressif RCP firmware + use serial adapter `ezsp` in Z2M |
| Battery-powered Zigbee sensor with deep sleep | This skill — `ESP_ZB_DEVICE_TYPE_ED` + ESP-IDF sleep API |
| Mains-powered Zigbee router/repeater | This skill — `ESP_ZB_DEVICE_TYPE_ROUTER` (less common, network coverage helper) |
| Use ZHA instead of Z2M | Out of scope — ZHA requires Python quirks files, not JS converters |

---

## Workflow Phases (recommended progression)

| Phase | What | Why |
|-------|------|-----|
| 0 | Blink test (no Zigbee) | Validate ESP-IDF toolchain, USB Serial/JTAG, LED GPIO before protocol complexity |
| 1 | Zigbee on/off light, no manuf/model | Validate pairing, channel scan, NVS persistence — Z2M lists as "Not supported", that's OK |
| 2 | Real sensor (DHT, BME, PIR) | Add second endpoint with sensor clusters (0x0402, 0x0405, 0x0500) |
| 3 | Manuf/model + external converter | Z2M matches `modelIdentifier`, applies converter, clean entity names |
| 4+ | Additional endpoints | Multi-cluster device — light + sensor + button on same device |

Detailed rationale per phase: [references/workflow.md](references/workflow.md)

---

## Critical Code Pattern — Manuf/Model injection

`esp_zb_on_off_light_ep_create()` (and other helpers) create the Basic cluster automatically but DO NOT include manufacturer name / model identifier. Inject them manually before `esp_zb_device_register`:

```c
#define MANUFACTURER_NAME    "\x0e""ClaudioJaraLab"   // 14 chars, length byte 0x0e
#define MODEL_IDENTIFIER     "\x0d""xiao-c6-light"    // 13 chars, length byte 0x0d

// AFTER esp_zb_on_off_light_ep_create, BEFORE esp_zb_device_register:
esp_zb_cluster_list_t *cluster_list = esp_zb_ep_list_get_ep(ep_list, HA_ENDPOINT);
esp_zb_attribute_list_t *basic_cluster = esp_zb_cluster_list_get_cluster(
    cluster_list, ESP_ZB_ZCL_CLUSTER_ID_BASIC, ESP_ZB_ZCL_CLUSTER_SERVER_ROLE);
esp_zb_basic_cluster_add_attr(basic_cluster,
    ESP_ZB_ZCL_ATTR_BASIC_MANUFACTURER_NAME_ID, MANUFACTURER_NAME);
esp_zb_basic_cluster_add_attr(basic_cluster,
    ESP_ZB_ZCL_ATTR_BASIC_MODEL_IDENTIFIER_ID, MODEL_IDENTIFIER);
```

Full skeleton with signal handler + attribute handler: [assets/main-skeleton.c](assets/main-skeleton.c)

---

## Z2M External Converter (Z2M 2.x)

Place at `/config/zigbee2mqtt/external_converters/<file>.js` (auto-loads on Z2M restart). Validate by checking MQTT topic `zigbee2mqtt/bridge/converters` — empty array means converter failed to load.

```javascript
const {onOff} = require('zigbee-herdsman-converters/lib/modernExtend');

module.exports = [{
    zigbeeModel: ['xiao-c6-light'],   // MUST match firmware MODEL_IDENTIFIER exactly
    model: 'xiao-c6-light',
    vendor: 'ClaudioJaraLab',         // MUST match firmware MANUFACTURER_NAME exactly
    description: 'Custom Zigbee light',
    extend: [onOff({powerOnBehavior: false})],
}];
```

After dropping the file: HA → Add-ons → Zigbee2MQTT → Restart → Z2M Web UI → device → **Re-interview** (NOT re-pair).

Pattern catalogue for sensors and multi-cluster devices: [references/z2m-converter-patterns.md](references/z2m-converter-patterns.md)

---

## Commands

```bash
# Setup (once per machine)
brew install cmake ninja dfu-util ccache
mkdir -p ~/esp && cd ~/esp
git clone -b v5.3.2 --recursive https://github.com/espressif/esp-idf.git
cd esp-idf && ./install.sh esp32c6
# Add to ~/.zshrc — note the SPACE between dot and tilde, dot is the source command:
#   alias get_idf='. ~/esp/esp-idf/export.sh'

# Per project
get_idf
idf.py set-target esp32c6           # esp32h2 if H2 board
idf.py build                        # first time tarda — descarga zboss + zigbee-lib (~cientos de MB)
ls /dev/cu.usbmodem*                # XIAO C6 has native USB-Serial-JTAG
idf.py -p /dev/cu.usbmodemXXXX flash monitor
# Exit monitor: Ctrl+]

# Wipe Zigbee join (factory reset)
idf.py -p /dev/cu.usbmodemXXXX erase-flash

# Z2M side (after editing converter or firmware manuf/model):
# 1. Drop converter in /config/zigbee2mqtt/external_converters/
# 2. HA → Settings → Add-ons → Zigbee2MQTT → Restart
# 3. Z2M Web UI → device → Re-interview
```

---

## Resources

- **Templates** in [assets/](assets/): `main-skeleton.c`, `partitions.csv`, `sdkconfig.defaults`, `idf_component.yml`, root + main `CMakeLists.txt`, `converter-skeleton.js`
- **Phase workflow**: [references/workflow.md](references/workflow.md)
- **Gotchas catalogue** (10 critical, vivos en sesión real): [references/gotchas.md](references/gotchas.md)
- **Cluster patterns** (on/off, temp, humidity, IAS Zone): [references/cluster-patterns.md](references/cluster-patterns.md)
- **Z2M converter patterns**: [references/z2m-converter-patterns.md](references/z2m-converter-patterns.md)
- **Working validated example** (XIAO C6 + SLZB-06): `/Users/claudiojara/Workspace/lab/experiments/esp32/projects/xiao-c6-zigbee/`
- **Engram memories** (`mem_search` with these topic_keys): `esp32/xiao-c6-zigbee-endpoint`, `esp32/esp-idf-zigbee-toolchain`

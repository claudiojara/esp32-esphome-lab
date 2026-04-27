# Phase Workflow

Recommended progression for any Zigbee endpoint project. Each phase isolates failure modes — if you skip ahead and something breaks, you don't know which layer to debug.

## Phase 0 — Blink test (no Zigbee)

**What**: Plain GPIO toggle. No Zigbee initialization. Validate ESP-IDF toolchain compiles, USB Serial/JTAG console outputs logs, and the LED on the assumed GPIO actually responds.

**Why**: If the LED doesn't blink in Phase 0, no amount of Zigbee debugging will save you in Phase 1. XIAO C6's user LED is on GPIO15 — verified physically. Other boards may differ. Confirm before building protocol layers on top.

**Done when**: User confirms LED blinks at the expected rate, AND `idf.py monitor` shows the log lines.

## Phase 1 — Zigbee on/off light, no manuf/model

**What**: Add Zigbee End Device role + on/off cluster on endpoint 10. Use `ESP_ZB_TRANSCEIVER_ALL_CHANNELS_MASK` (16-channel scan). NO manufacturer name / model identifier yet.

**Why**: The first join is the hardest. Multi-step handshake (Beacon Request → Association → Network Key transfer → Device Announce). If any step fails, the join aborts. Scanning all channels removes "wrong channel" from the suspect list. Z2M will list the device as "Not supported" — that's expected and fine.

**Done when**: Monitor shows `Joined network — Ext PAN ID xx:xx:..., PAN 0xXXXX, Channel N, Short 0xXXXX` AND HA shows a generic switch entity AND toggling from HA flips the LED.

## Phase 2 — Real sensor (DHT, BME, PIR, etc.)

**What**: Add a second endpoint (HA_ENDPOINT 11) with sensor-appropriate clusters. Common: `0x0402` (Temperature Measurement), `0x0405` (Relative Humidity), `0x0500` (IAS Zone for binary contact/motion). Read sensor in a separate FreeRTOS task, periodically update attributes via `esp_zb_zcl_set_attribute_val()`.

**Why**: Splitting endpoints (10 light, 11 sensor) is the canonical Zigbee multi-function pattern. Z2M reads endpoint metadata at interview and exposes each as separate entities. Easier to debug than cramming clusters into one endpoint.

**Done when**: Monitor shows attribute updates AND Z2M reports the values (still as "Not supported" if no converter yet).

## Phase 3 — Manuf/Model + external converter

**What**: Inject `MANUFACTURER_NAME` and `MODEL_IDENTIFIER` into Basic cluster (see SKILL.md critical pattern). Drop a `.js` converter in `/config/zigbee2mqtt/external_converters/`. Restart Z2M. Re-interview the device.

**Why**: Without the converter match, Z2M auto-generates ugly entity names (`switch.0xIEEEADDR`) and ugly device labels. The converter teaches Z2M "this device is supported, expose its clusters as these HA entities". Branding + cleaner UX in HA.

**Done when**: Z2M shows "Soportado: external" badge AND device manufacturer/model match the strings in firmware AND HA entities are renamed to `<friendly_name>` based.

## Phase 4+ — Additional endpoints

**What**: Add more endpoints for additional sensors/actuators (button, tilt, second temperature, etc.). Each endpoint has its own cluster list and its own entity exposes in the converter.

**Why**: Better than one fat endpoint with 10 clusters — Zigbee profile matching (HA / ZLL / ZHA) prefers homogeneous endpoints. Z2M handles multi-endpoint devices natively.

**Done when**: Each new endpoint reports correctly AND HA shows separate entities for each function.

---

## Iteration golden rules

- **Always finish Phase N before starting N+1.** Don't add a sensor while Zigbee is broken. Don't add a converter while attrs are wrong.
- **Use `idf.py erase-flash` only between phases when changing partition layout** OR when you need to drop the join entirely. Mid-phase iterations: plain `idf.py flash` preserves NVS and the device just reconnects.
- **`Re-interview` (NOT re-pair) when changing only Basic cluster contents** (manuf/model, sw version). Re-pair (factory reset + permit_join) only when changing endpoint count, cluster list, or device profile.

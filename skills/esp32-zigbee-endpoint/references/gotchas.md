# Gotchas Catalogue

Ten gotchas vivos — pisados en sesión real al construir un Zigbee endpoint en XIAO ESP32-C6 + SLZB-06 + Z2M 2.9.2.

## 1. XIAO ESP32-C6 user LED is GPIO15, active LOW

`gpio_set_level(15, 0)` turns the LED ON. Inverted from intuition. Boot button is on GPIO9 (used for manual bootloader entry: hold BOOT, press RESET, release BOOT, then `idf.py flash`). Other ESP32-C6 boards (DevKitC-1, DevKitM-1) have different LED pins — always verify with a Phase 0 blink before building Zigbee on top.

## 2. XIAO C6 has native USB — needs USB Serial/JTAG console

XIAO ESP32-C6 has no CP2102/CH340 UART bridge — it uses the ESP32-C6's built-in USB-Serial-JTAG. Required: `CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y` in sdkconfig.defaults. Without it, `idf.py monitor` shows nothing. Port appears as `/dev/cu.usbmodemXXXX` on macOS (no SLAB / wchusbserial prefix).

## 3. External antenna is critical for the FIRST join

Zigbee join is a multi-step RF handshake (Beacon Request → Beacon → Association Request → Association Response → Network Key transfer → Device Announce). Each step needs an RF round-trip without packet loss. XIAO C6's PCB chip antenna alone often fails the chain — first join attempts time out or `Network steering failed (ESP_FAIL)`. Connect an external u.FL antenna for the join, then once the device is on the network the keys are stored in NVS and the link survives at low LQI (40-65). For production, keep the antenna.

## 4. During development, scan ALL channels

`esp_zb_set_primary_network_channel_set(ESP_ZB_TRANSCEIVER_ALL_CHANNELS_MASK)` scans channels 11-26 (~140ms each, ~2.2s total). It's slower per attempt but bulletproof — works regardless of the coordinator's actual channel. After confirming the device joins reliably, narrow to a single channel for faster steering: `esp_zb_set_primary_network_channel_set(1u << 11)` for channel 11. **Suspicious symptom**: steering "fails" in <200ms — that's not a real scan, that's a precondition failing. If it happens, double-check radio init and channel mask validity.

## 5. `erase-flash` semantics (read this twice)

`idf.py erase-flash` wipes ALL flash including the Zigbee NVS partition `zb_storage`. That nukes the network join. Use it ONLY when:
- First flash of a fresh chip
- Want to factory-reset and re-pair to a different network
- Suspect NVS corruption from a botched previous flash

For ALL OTHER iterations (firmware tweaks, attribute changes, code refactors): plain `idf.py flash` preserves the join. The device reboots, sees `non-factory-new` state, fires `ESP_ZB_BDB_SIGNAL_DEVICE_REBOOT`, and rejoins automatically with the network keys it remembers.

## 6. Re-interview ≠ re-pair

These are DIFFERENT operations and conflating them costs time:

| Operation | Trigger in Z2M | When to use |
|-----------|----------------|-------------|
| **Re-interview** | Z2M Web UI → device → Re-interview | Firmware changed only Basic cluster contents (manufacturer name, model identifier, sw version). Device stays on the network, friendly_name preserved, HA entities preserved. |
| **Re-pair** | Remove device + factory reset chip + permit_join + boot | Firmware changed endpoint count, cluster list, or device type. Device rejoins from scratch, friendly_name lost, HA entities recreated. |

Default to re-interview. Only escalate to re-pair if re-interview fails.

## 7. ZCL strings are length-prefixed (NOT null-terminated)

The Zigbee Cluster Library defines string attributes as: first byte = length in characters, then ASCII chars (no null terminator). C representation:

```c
#define MANUFACTURER_NAME    "\x0e""ClaudioJaraLab"   // 0x0e = 14 chars
#define MODEL_IDENTIFIER     "\x0d""xiao-c6-light"    // 0x0d = 13 chars
```

Count the characters. Off-by-one on the length byte → string truncated or garbage chars in Z2M → Z2M can't match the converter (since `zigbeeModel: ['xiao-c6-light']` won't equal `'xiao-c6-ligh'`). Convert string length to hex carefully:

| Chars | Hex | Chars | Hex |
|-------|-----|-------|-----|
| 8 | `\x08` | 16 | `\x10` |
| 10 | `\x0a` | 20 | `\x14` |
| 13 | `\x0d` | 24 | `\x18` |
| 14 | `\x0e` | 32 | `\x20` |

## 8. Zigbee NVS partitions are mandatory

`partitions.csv` MUST include:

```
zb_storage,    data, fat,     <offset>, 0x4000,
zb_fct,        data, fat,     <offset>, 0x1000,
```

Missing them → device boots, joins fine on first attempt, but on reboot acts factory-new again (NVS lost the keys). User chases ghosts. The sample partition table in `assets/partitions.csv` has them at 0x190000 and 0x194000 for a 4MB flash with 1.5MB factory app — adjust offsets if you change app size.

## 9. Stack init noise during reboot reflash is normal

After a non-erase reflash (firmware change but join preserved), the boot logs include:

```
W zb_light: Stack init failed (ESP_FAIL), retry in 1s
E ESP_ZIGBEE_CORE: In BDB commissioning, an error occurred (for example: the device has already been running)
E zb_light: bdb_start_top_level_commissioning_cb(30): Failed to start commissioning
```

This is the ZBoss scheduler trying to (re)commission a device that's already on the network. It eventually settles into:

```
I zb_light: Deferred driver initialization non-factory-new
I zb_light: Device rebooted — already on network
```

Inocuous. Don't chase it.

## 10. Z2M topic `zigbee2mqtt/bridge/converters` validates external converters

After dropping a `.js` file in `/config/zigbee2mqtt/external_converters/` and restarting Z2M, look at MQTT topic `zigbee2mqtt/bridge/converters`:

- Empty array `[]` or no message → converter failed to load (syntax error, MODULE_NOT_FOUND, etc.)
- Array with your converter object → loaded OK, ready for re-interview

Faster than scrolling Z2M logs. Subscribe via Mosquitto CLI, MQTT Explorer, or HA → Developer Tools → MQTT → Listen.

# Zigbee2MQTT External Converter Patterns (Z2M 2.x)

Reference for common cluster-to-entity mappings using `modernExtend`. Apply to `/config/zigbee2mqtt/external_converters/<name>.js`.

## File anatomy

```javascript
const {onOff, temperature, humidity, /*...*/ } = require('zigbee-herdsman-converters/lib/modernExtend');

module.exports = [{
    zigbeeModel: ['xiao-c6-light'],   // EXACT match with firmware MODEL_IDENTIFIER
    model: 'xiao-c6-light',           // shows in Z2M UI
    vendor: 'ClaudioJaraLab',         // EXACT match with firmware MANUFACTURER_NAME
    description: 'Custom Zigbee endpoint',
    extend: [
        // one or more modernExtend builders
    ],
}];
```

`zigbeeModel` is an array because some manufacturers ship multiple firmware variants under the same converter. For one-off custom devices, use a single-element array.

## modernExtend builders — quick reference

| Builder | What it does | Common options |
|---------|--------------|----------------|
| `onOff()` | Cluster 0x0006 → switch entity | `{powerOnBehavior: false}` if firmware doesn't implement startUpOnOff attr |
| `temperature()` | Cluster 0x0402 → sensor entity (°C) | `{precision: 1, reporting: {min: 30, max: 300, change: 50}}` |
| `humidity()` | Cluster 0x0405 → sensor entity (%) | Same reporting shape |
| `pressure()` | Cluster 0x0403 → sensor entity (hPa) | — |
| `illuminance()` | Cluster 0x0400 → sensor entity (lux) | — |
| `iasZoneAlarm()` | Cluster 0x0500 → binary_sensor | `{zoneType: 'contact', zoneAttributes: ['alarm_1']}` |
| `battery()` | Cluster 0x0001 → diagnostics | `{voltage: true, voltageReporting: true, percentage: true}` |
| `identify()` | Cluster 0x0003 → identify button | — |
| `electricityMeter()` | Cluster 0x0702 → power/energy sensor | — |
| `light()` | Color/dimmer light (clusters 0x0008 + 0x0300) | `{colorTemp: {range: [153, 500]}, color: true}` |

## Multi-endpoint devices

When firmware uses multiple endpoints (e.g., 10 = light, 11 = sensor), the converter applies extends across the whole device by default. To target specific endpoints:

```javascript
const {deviceEndpoints, onOff, temperature} = require('zigbee-herdsman-converters/lib/modernExtend');

module.exports = [{
    zigbeeModel: ['my-multi-device'],
    model: 'my-multi-device',
    vendor: 'ClaudioJaraLab',
    description: 'Light + temperature on one Zigbee device',
    extend: [
        deviceEndpoints({endpoints: {light: 10, sensor: 11}}),
        onOff({endpointNames: ['light']}),
        temperature({endpointNames: ['sensor']}),
    ],
}];
```

The `endpointNames` option ties the modernExtend to the named endpoints declared in `deviceEndpoints`.

## Attribute reporting

`modernExtend` builders auto-configure reporting. For tighter control:

```javascript
temperature({
    reporting: {
        min: 10,        // seconds: minimum interval between reports
        max: 600,       // seconds: maximum interval (heartbeat)
        change: 50,     // value: minimum delta to trigger early report (here = 0.5 °C since 0.01 °C units)
    },
})
```

Tradeoff: small `min` + small `change` = responsive but more battery drain. Large `min` = lazy reports.

## Custom converters (when modernExtend is not enough)

For exotic clusters, custom commands, or firmware-specific quirks, use the legacy `fromZigbee` (fz) and `toZigbee` (tz) arrays:

```javascript
const fz = require('zigbee-herdsman-converters/converters/fromZigbee');
const tz = require('zigbee-herdsman-converters/converters/toZigbee');
const e = require('zigbee-herdsman-converters/lib/exposes').presets;

module.exports = [{
    zigbeeModel: ['custom-thing'],
    model: 'custom-thing',
    vendor: 'ClaudioJaraLab',
    description: 'Device with non-standard cluster',
    fromZigbee: [fz.on_off, /* custom converters */],
    toZigbee: [tz.on_off, /* custom converters */],
    exposes: [e.switch(), /* e.binary(), e.numeric(), e.enum() */],
}];
```

Mix-and-match: you can combine `extend: [...]` (modern) and `fromZigbee/toZigbee/exposes` (legacy) in the same definition. Use the legacy arrays only for what modernExtend can't express.

## Validation

After dropping the file and restarting Z2M:

1. **MQTT topic check**: Subscribe to `zigbee2mqtt/bridge/converters`. The published payload should include an entry for your file (`name`, `code`).
2. **Z2M log check**: Look for any `MODULE_NOT_FOUND` or `SyntaxError` near the converter file path.
3. **Device interview**: After Z2M restart, click **Re-interview** on the device. Outcome:
   - "Soportado: external" badge → match working
   - Still "Not supported" → check that `zigbeeModel` matches firmware `MODEL_IDENTIFIER` BYTE-FOR-BYTE (case sensitive, no extra spaces)

## Common mismatch causes

| Symptom | Cause |
|---------|-------|
| Z2M shows "Not supported" with empty model | Firmware doesn't actually advertise manuf/model — verify Basic cluster has both attrs |
| Z2M shows "Not supported" with model that LOOKS right | Length-prefix off-by-one in firmware ZCL string truncated the value (see gotchas.md #7) |
| Converter loads but exposes nothing | Endpoint mismatch — converter expects endpoint 1 but firmware uses 10. Use `deviceEndpoints({endpoints: {default: 10}})` |
| Converter loads but commands don't reach device | `toZigbee` missing for that cluster, or device doesn't actually expose the cluster as server role |

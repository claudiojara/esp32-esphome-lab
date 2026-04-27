/*
 * Z2M 2.x external converter skeleton.
 *
 * Place at /config/zigbee2mqtt/external_converters/<file>.js — auto-loads on Z2M restart.
 * Validate: after restart, MQTT topic zigbee2mqtt/bridge/converters should include this file.
 *
 * zigbeeModel + vendor MUST match firmware MODEL_IDENTIFIER + MANUFACTURER_NAME exactly
 * (ZCL strings without their length-prefix byte). Mismatch → device shown as "Not supported".
 */

const {
    onOff,
    temperature,
    humidity,
    iasZoneAlarm,
    battery,
    identify,
} = require('zigbee-herdsman-converters/lib/modernExtend');

module.exports = [{
    zigbeeModel: ['xiao-c6-light'],          // EXACT match with firmware MODEL_IDENTIFIER
    model: 'xiao-c6-light',
    vendor: 'ClaudioJaraLab',                // EXACT match with firmware MANUFACTURER_NAME
    description: 'Custom Zigbee endpoint',
    extend: [
        onOff({powerOnBehavior: false}),     // simple switch / light
        // temperature(),                    // exposes 0x0402 cluster as e.temperature()
        // humidity(),                       // exposes 0x0405 cluster as e.humidity()
        // iasZoneAlarm({zoneType: 'contact', zoneAttributes: ['alarm_1']}),  // 0x0500 IAS Zone
        // battery({voltage: true, voltageReporting: true}),
        // identify(),
    ],
}];

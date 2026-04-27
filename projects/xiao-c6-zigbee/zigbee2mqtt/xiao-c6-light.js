const {onOff} = require('zigbee-herdsman-converters/lib/modernExtend');

module.exports = [{
    zigbeeModel: ['xiao-c6-light'],
    model: 'xiao-c6-light',
    vendor: 'ClaudioJaraLab',
    description: 'XIAO ESP32-C6 Zigbee on/off light (custom firmware)',
    extend: [onOff({powerOnBehavior: false})],
}];

# Common Zigbee Cluster Patterns (esp-zigbee-sdk)

Snippets for the clusters used most often in HA-integrated endpoints. Each section: cluster ID, when to use, firmware (C) snippet, attribute reporting, and a pointer to the matching Z2M converter pattern.

## On/Off — cluster `0x0006`

For switches, lights, relays, anything binary state.

### Firmware (helper-based)

```c
#include "ha/esp_zigbee_ha_standard.h"

esp_zb_on_off_light_cfg_t light_cfg = ESP_ZB_DEFAULT_ON_OFF_LIGHT_CONFIG();
esp_zb_ep_list_t *ep_list = esp_zb_on_off_light_ep_create(HA_ENDPOINT, &light_cfg);
// (inject manuf/model into Basic cluster — see SKILL.md)
esp_zb_device_register(ep_list);
```

### Receive on/off command from coordinator

```c
// In zb_attribute_handler:
if (msg->info.cluster == ESP_ZB_ZCL_CLUSTER_ID_ON_OFF &&
    msg->attribute.id == ESP_ZB_ZCL_ATTR_ON_OFF_ON_OFF_ID &&
    msg->attribute.data.type == ESP_ZB_ZCL_ATTR_TYPE_BOOL) {
    bool on = *(bool *)msg->attribute.data.value;
    apply_state(on);
}
```

### Z2M converter

`extend: [onOff({powerOnBehavior: false})]`

---

## Temperature Measurement — cluster `0x0402`

Reports temperature in 0.01 °C units (signed 16-bit). E.g., `2350` = 23.50 °C.

### Firmware (build cluster manually for sensor-only endpoint)

```c
esp_zb_temperature_meas_cluster_cfg_t temp_cfg = {
    .measured_value     = 0,
    .min_value          = -2000,    // -20.00 °C
    .max_value          = 8000,     // 80.00 °C
};
esp_zb_attribute_list_t *temp_cluster = esp_zb_temperature_meas_cluster_create(&temp_cfg);
// add to cluster_list with esp_zb_cluster_list_add_temperature_meas_cluster(...)
```

### Update attribute periodically (in your sensor task)

```c
int16_t temp_raw = (int16_t)(temp_celsius * 100);
esp_zb_lock_acquire(portMAX_DELAY);
esp_zb_zcl_set_attribute_val(SENSOR_ENDPOINT,
    ESP_ZB_ZCL_CLUSTER_ID_TEMP_MEASUREMENT,
    ESP_ZB_ZCL_CLUSTER_SERVER_ROLE,
    ESP_ZB_ZCL_ATTR_TEMP_MEASUREMENT_VALUE_ID,
    &temp_raw, false);
esp_zb_lock_release();
```

`esp_zb_lock_*` is required — sensor task and Zigbee task share state.

### Z2M converter

`extend: [temperature()]`

---

## Relative Humidity — cluster `0x0405`

Reports humidity in 0.01% units. E.g., `5500` = 55.00%.

### Firmware

```c
esp_zb_humidity_meas_cluster_cfg_t hum_cfg = {
    .measured_value = 0,
    .min_value      = 0,
    .max_value      = 10000,    // 100.00%
};
esp_zb_attribute_list_t *hum_cluster = esp_zb_humidity_meas_cluster_create(&hum_cfg);
```

### Update

```c
uint16_t hum_raw = (uint16_t)(humidity_pct * 100);
esp_zb_lock_acquire(portMAX_DELAY);
esp_zb_zcl_set_attribute_val(SENSOR_ENDPOINT,
    ESP_ZB_ZCL_CLUSTER_ID_REL_HUMIDITY_MEASUREMENT,
    ESP_ZB_ZCL_CLUSTER_SERVER_ROLE,
    ESP_ZB_ZCL_ATTR_REL_HUMIDITY_MEASUREMENT_VALUE_ID,
    &hum_raw, false);
esp_zb_lock_release();
```

### Z2M converter

`extend: [humidity()]`

---

## IAS Zone — cluster `0x0500`

Binary security/contact sensors (door, window, motion, tilt, vibration). Different from a plain on/off cluster — IAS Zone has its own enrollment handshake with the coordinator.

### Firmware

```c
esp_zb_ias_zone_cluster_cfg_t ias_cfg = {
    .zone_state     = 0,                // not enrolled
    .zone_type      = 0x0015,           // contact switch (0x000d motion, 0x002d vibration)
    .zone_status    = 0,                // alarm bits
    .ias_cie_addr   = 0,                // set after enrollment
    .zone_id        = 0xff,
};
esp_zb_attribute_list_t *ias_cluster = esp_zb_ias_zone_cluster_create(&ias_cfg);
```

### Update zone status when sensor triggers

```c
uint16_t status = 0x0001;   // alarm 1 active = contact OPEN / motion DETECTED / etc
esp_zb_lock_acquire(portMAX_DELAY);
esp_zb_zcl_set_attribute_val(SENSOR_ENDPOINT,
    ESP_ZB_ZCL_CLUSTER_ID_IAS_ZONE,
    ESP_ZB_ZCL_CLUSTER_SERVER_ROLE,
    ESP_ZB_ZCL_ATTR_IAS_ZONE_ZONESTATUS_ID,
    &status, false);
esp_zb_lock_release();
```

Plus handle the enrollment callback (`ESP_ZB_CORE_IAS_ZONE_ENROLL_RESPONSE_VALUE_CB_ID`) in `zb_action_handler`.

### Z2M converter

`extend: [iasZoneAlarm({zoneType: 'contact', zoneAttributes: ['alarm_1']})]`

For motion: `zoneType: 'occupancy'`. For vibration: `'vibration'`.

---

## Power Configuration / Battery — cluster `0x0001`

For battery-powered End Devices. Reports voltage and percentage remaining.

### Firmware

```c
esp_zb_power_config_cluster_cfg_t pwr_cfg = {
    .main_voltage   = 30,        // 0.1 V units (3.0 V)
    .main_freq      = 0,
    .main_alarm_mask = 0,
    .main_voltage_min  = 25,     // 2.5 V
    .main_voltage_max  = 35,     // 3.5 V
};
```

Update battery_voltage and battery_percentage_remaining attrs periodically.

### Z2M converter

`extend: [battery({voltage: true, voltageReporting: true, percentage: true})]`

---

## Multi-cluster device — multiple endpoints

Don't pile clusters into one endpoint. Use one endpoint per logical function:

```c
// Endpoint 10 — light
esp_zb_ep_list_t *ep_list = esp_zb_on_off_light_ep_create(10, &light_cfg);
// (inject manuf/model into endpoint 10's Basic cluster)

// Endpoint 11 — temperature + humidity sensor
esp_zb_cluster_list_t *sensor_clusters = esp_zb_zcl_cluster_list_create();
esp_zb_cluster_list_add_temperature_meas_cluster(sensor_clusters, temp_cluster, ESP_ZB_ZCL_CLUSTER_SERVER_ROLE);
esp_zb_cluster_list_add_humidity_meas_cluster(sensor_clusters, hum_cluster, ESP_ZB_ZCL_CLUSTER_SERVER_ROLE);
esp_zb_endpoint_config_t ep11_cfg = {
    .endpoint = 11,
    .app_profile_id = ESP_ZB_AF_HA_PROFILE_ID,
    .app_device_id = ESP_ZB_HA_TEMPERATURE_SENSOR_DEVICE_ID,
    .app_device_version = 0,
};
esp_zb_ep_list_add_ep(ep_list, sensor_clusters, ep11_cfg);

esp_zb_device_register(ep_list);
```

Z2M reads metadata from BOTH endpoints during interview and exposes them as separate entities.

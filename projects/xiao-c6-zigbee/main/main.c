#include "esp_check.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "esp_zigbee_core.h"
#include "ha/esp_zigbee_ha_standard.h"

#define LED_GPIO                    GPIO_NUM_15
#define LED_ACTIVE_LOW              1

#define HA_ENDPOINT                 10
#define INSTALLCODE_POLICY_ENABLE   false
#define ED_AGING_TIMEOUT            ESP_ZB_ED_AGING_TIMEOUT_64MIN
#define ED_KEEP_ALIVE               3000

// ZCL strings: first byte is length, then ASCII (no null terminator)
#define MANUFACTURER_NAME           "\x0e""ClaudioJaraLab"
#define MODEL_IDENTIFIER            "\x0d""xiao-c6-light"

static const char *TAG = "zb_light";

static void light_set_state(bool on) {
    gpio_set_level(LED_GPIO, LED_ACTIVE_LOW ? !on : on);
    ESP_LOGI(TAG, "Light %s", on ? "ON" : "OFF");
}

static void bdb_start_top_level_commissioning_cb(uint8_t mode_mask) {
    ESP_RETURN_ON_FALSE(esp_zb_bdb_start_top_level_commissioning(mode_mask) == ESP_OK,
                        , TAG, "Failed to start commissioning");
}

void esp_zb_app_signal_handler(esp_zb_app_signal_t *signal_struct) {
    uint32_t *p_sg_p = signal_struct->p_app_signal;
    esp_err_t err_status = signal_struct->esp_err_status;
    esp_zb_app_signal_type_t sig_type = *p_sg_p;

    switch (sig_type) {
        case ESP_ZB_ZDO_SIGNAL_SKIP_STARTUP:
            ESP_LOGI(TAG, "Initialize Zigbee stack");
            esp_zb_bdb_start_top_level_commissioning(ESP_ZB_BDB_MODE_INITIALIZATION);
            break;

        case ESP_ZB_BDB_SIGNAL_DEVICE_FIRST_START:
        case ESP_ZB_BDB_SIGNAL_DEVICE_REBOOT:
            if (err_status == ESP_OK) {
                ESP_LOGI(TAG, "Deferred driver initialization %s",
                         esp_zb_bdb_is_factory_new() ? "factory-new" : "non-factory-new");
                if (esp_zb_bdb_is_factory_new()) {
                    ESP_LOGI(TAG, "Start network steering");
                    esp_zb_bdb_start_top_level_commissioning(ESP_ZB_BDB_MODE_NETWORK_STEERING);
                } else {
                    ESP_LOGI(TAG, "Device rebooted — already on network");
                }
            } else {
                ESP_LOGW(TAG, "Stack init failed (%s), retry in 1s", esp_err_to_name(err_status));
                esp_zb_scheduler_alarm((esp_zb_callback_t)bdb_start_top_level_commissioning_cb,
                                       ESP_ZB_BDB_MODE_INITIALIZATION, 1000);
            }
            break;

        case ESP_ZB_BDB_SIGNAL_STEERING:
            if (err_status == ESP_OK) {
                esp_zb_ieee_addr_t epid;
                esp_zb_get_extended_pan_id(epid);
                ESP_LOGI(TAG, "Joined network — Ext PAN ID %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x, "
                              "PAN 0x%04hx, Channel %d, Short 0x%04hx",
                         epid[7], epid[6], epid[5], epid[4], epid[3], epid[2], epid[1], epid[0],
                         esp_zb_get_pan_id(), esp_zb_get_current_channel(), esp_zb_get_short_address());
            } else {
                ESP_LOGW(TAG, "Network steering failed (%s), retry in 1s", esp_err_to_name(err_status));
                esp_zb_scheduler_alarm((esp_zb_callback_t)bdb_start_top_level_commissioning_cb,
                                       ESP_ZB_BDB_MODE_NETWORK_STEERING, 1000);
            }
            break;

        default:
            ESP_LOGI(TAG, "ZDO signal %s (0x%x), status %s",
                     esp_zb_zdo_signal_to_string(sig_type), sig_type, esp_err_to_name(err_status));
            break;
    }
}

static esp_err_t zb_attribute_handler(const esp_zb_zcl_set_attr_value_message_t *msg) {
    ESP_RETURN_ON_FALSE(msg, ESP_FAIL, TAG, "empty message");
    ESP_RETURN_ON_FALSE(msg->info.status == ESP_ZB_ZCL_STATUS_SUCCESS,
                        ESP_ERR_INVALID_ARG, TAG, "attr message status %d", msg->info.status);

    ESP_LOGI(TAG, "Attr write — ep %d, cluster 0x%x, attr 0x%x, size %d",
             msg->info.dst_endpoint, msg->info.cluster, msg->attribute.id, msg->attribute.data.size);

    if (msg->info.dst_endpoint == HA_ENDPOINT &&
        msg->info.cluster == ESP_ZB_ZCL_CLUSTER_ID_ON_OFF &&
        msg->attribute.id == ESP_ZB_ZCL_ATTR_ON_OFF_ON_OFF_ID &&
        msg->attribute.data.type == ESP_ZB_ZCL_ATTR_TYPE_BOOL) {
        bool on = msg->attribute.data.value ? *(bool *)msg->attribute.data.value : false;
        light_set_state(on);
    }
    return ESP_OK;
}

static esp_err_t zb_action_handler(esp_zb_core_action_callback_id_t cb_id, const void *msg) {
    switch (cb_id) {
        case ESP_ZB_CORE_SET_ATTR_VALUE_CB_ID:
            return zb_attribute_handler((esp_zb_zcl_set_attr_value_message_t *)msg);
        default:
            ESP_LOGW(TAG, "Unhandled core action 0x%x", cb_id);
            return ESP_OK;
    }
}

static void zb_task(void *arg) {
    esp_zb_cfg_t zb_nwk_cfg = {
        .esp_zb_role = ESP_ZB_DEVICE_TYPE_ED,
        .install_code_policy = INSTALLCODE_POLICY_ENABLE,
        .nwk_cfg.zed_cfg = {
            .ed_timeout = ED_AGING_TIMEOUT,
            .keep_alive = ED_KEEP_ALIVE,
        },
    };
    esp_zb_init(&zb_nwk_cfg);

    esp_zb_on_off_light_cfg_t light_cfg = ESP_ZB_DEFAULT_ON_OFF_LIGHT_CONFIG();
    esp_zb_ep_list_t *ep_list = esp_zb_on_off_light_ep_create(HA_ENDPOINT, &light_cfg);

    esp_zb_cluster_list_t *cluster_list = esp_zb_ep_list_get_ep(ep_list, HA_ENDPOINT);
    esp_zb_attribute_list_t *basic_cluster = esp_zb_cluster_list_get_cluster(
        cluster_list, ESP_ZB_ZCL_CLUSTER_ID_BASIC, ESP_ZB_ZCL_CLUSTER_SERVER_ROLE);
    esp_zb_basic_cluster_add_attr(basic_cluster,
        ESP_ZB_ZCL_ATTR_BASIC_MANUFACTURER_NAME_ID, MANUFACTURER_NAME);
    esp_zb_basic_cluster_add_attr(basic_cluster,
        ESP_ZB_ZCL_ATTR_BASIC_MODEL_IDENTIFIER_ID, MODEL_IDENTIFIER);

    esp_zb_device_register(ep_list);
    esp_zb_core_action_handler_register(zb_action_handler);

    esp_zb_set_primary_network_channel_set(ESP_ZB_TRANSCEIVER_ALL_CHANNELS_MASK);  // scan 11-26

    ESP_ERROR_CHECK(esp_zb_start(false));
    esp_zb_stack_main_loop();
}

void app_main(void) {
    ESP_LOGI(TAG, "XIAO ESP32-C6 — Zigbee on/off light starting");

    gpio_reset_pin(LED_GPIO);
    gpio_set_direction(LED_GPIO, GPIO_MODE_OUTPUT);
    light_set_state(false);

    ESP_ERROR_CHECK(nvs_flash_init());

    esp_zb_platform_config_t cfg = {
        .radio_config  = { .radio_mode = ZB_RADIO_MODE_NATIVE },
        .host_config   = { .host_connection_mode = ZB_HOST_CONNECTION_MODE_NONE },
    };
    ESP_ERROR_CHECK(esp_zb_platform_config(&cfg));

    xTaskCreate(zb_task, "zb_task", 4096, NULL, 5, NULL);
}

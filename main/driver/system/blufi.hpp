#pragma once

#include "soc/soc_caps.h"

#include "esp_err.h"
#include "esp_log.h"

#include "esp_blufi_api.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"
#include "esp_wifi.h"
#include "esp_log.h"

#if CONFIG_BT_CONTROLLER_ENABLED || !CONFIG_BT_NIMBLE_ENABLED
#include "esp_bt.h"
#endif
#ifdef CONFIG_BT_BLUEDROID_ENABLED
#include "esp_bt_main.h"
#include "esp_bt_device.h"
#include "esp_gap_ble_api.h"
#endif
#ifdef CONFIG_BT_NIMBLE_ENABLED
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "host/util/util.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"
#include "console/console.h"
#endif

#include "esp_err.h"
#include "esp_blufi_api.h"
#include "esp_log.h"

#include "../../quimblos/driver.hpp"
#include "wifi.hpp"

#ifndef CONFIG_SOC_BLUFI_SUPPORTED
#error "This SOC does not support BLUFI"
#endif

namespace qb {

    namespace driver {
    
        class BluFi: public qb::Driver {
    
            static WiFi* wifi;
    
            static esp_blufi_callbacks_t callbacks;
    
            static bool ble_is_connected;
            static esp_blufi_extra_info_t sta_conn_info;
    
            public:
                static char device_name[];
    
                BluFi(WiFi& wifi) {
                    BluFi::wifi = &wifi;
                    wifi.config.sta_auto_connect = true;
                }
    
                esp_err_t load() {
                    esp_err_t ret;
    
                    // BT Controller
                    #if CONFIG_BT_CONTROLLER_ENABLED || !CONFIG_BT_NIMBLE_ENABLED
                        ret = init_controller();
                        if (ret) {
                            ESP_LOGE(TAG, "Controller init failed: %s\n", esp_err_to_name(ret));
                            return ret;
                        }
                    #endif
    
                    // BT Host
                    ret = init_host(&callbacks);
                    if (ret) {
                        ESP_LOGE(TAG, "Host init failed: %s\n", esp_err_to_name(ret));
                        return ret;
                    }
    
                    #if !SOC_MPI_SUPPORTED
                        blufi_dh_pregen_start();
                        blufi_dh_pregen_wait();
                        esp_blufi_adv_start();
                    #endif
    
                    ESP_LOGI(TAG, "VERSION %04x\n", esp_blufi_get_version());
                    return ESP_OK;
                }
    
                esp_err_t unload() {
                    return ESP_OK;
                }
    
                static const char* TAG;
    
            private:
    
                /* security */
    
                #if !SOC_MPI_SUPPORTED
                    static void dh_pregen_start(void);
                    static void dh_pregen_start_with_cb(void (*done_cb)(void));
                    static void dh_pregen_wait(void);
                #endif
                
                static void dh_negotiate_data_handler(uint8_t *data, int len, uint8_t **output_data, int *output_len, bool *need_free);
                static int aes_encrypt(uint8_t iv8, uint8_t *crypt_data, int crypt_len);
                static int aes_decrypt(uint8_t iv8, uint8_t *crypt_data, int crypt_len);
                static uint16_t crc_checksum(uint8_t iv8, uint8_t *data, int len);
    
                static esp_err_t init_security(void);
                static void deinit_security(void);
    
                /* init */
    
                static esp_err_t gap_register_callback(void);
                static esp_err_t init_host(esp_blufi_callbacks_t *callbacks);
                static esp_err_t deinit_host();
                static esp_err_t init_controller();
                static esp_err_t deinit_controller();
    
                /* events */
    
                static void event_callback(esp_blufi_cb_event_t event, esp_blufi_cb_param_t *param);
        };
    
    }
}

#pragma once

#include <cstdint>
#include <string>
#include "nvs.hpp"
#include "esp_wifi.h"
#include "esp_err.h"

#include "../../quimblos/driver.hpp"

#if CONFIG_ESP_WIFI_AUTH_OPEN
#define QUIMBLOS_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_OPEN
#elif CONFIG_ESP_WIFI_AUTH_WEP
#define QUIMBLOS_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WEP
#elif CONFIG_ESP_WIFI_AUTH_WPA_PSK
#define QUIMBLOS_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA2_PSK
#define QUIMBLOS_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA_WPA2_PSK
#define QUIMBLOS_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA_WPA2_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA3_PSK
#define QUIMBLOS_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA3_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA2_WPA3_PSK
#define QUIMBLOS_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_WPA3_PSK
#elif CONFIG_ESP_WIFI_AUTH_WAPI_PSK
#define QUIMBLOS_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WAPI_PSK
#endif

#define QUIMBLOS_WIFI_CONNECTION_MAXIMUM_RETRY CONFIG_QUIMBLOS_WIFI_CONNECTION_MAXIMUM_RETRY

namespace driver {

    class WiFi: public quimblos::Driver {

        static EventGroupHandle_t wifi_event_group;

        public:

            static wifi_config_t sta_config;
            static wifi_config_t ap_config;
            
            static struct STA {
                uint8_t connect_retry;
                bool connected;
                bool got_ip;
                bool is_connecting;
                std::string bssid;
                std::string ssid;
            } sta;

            static struct AP {
                wifi_sta_list_t sta_list;
            } ap;

            static struct Config {
                bool sta_auto_connect;
                uint8_t sta_connect_max_retries;
            } config;

        public:
            WiFi(
                NVS& nvs,
                bool sta_auto_connect = false,
                uint8_t sta_connect_max_retries = 3
            ) {
                WiFi::config.sta_auto_connect = sta_auto_connect;
                WiFi::config.sta_connect_max_retries = sta_connect_max_retries;
            }

            esp_err_t load() {

                // Initialize WiFi chip
                ESP_ERROR_CHECK(esp_netif_init());
                wifi_event_group = xEventGroupCreate();
                ESP_ERROR_CHECK(esp_event_loop_create_default());
                
                esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
                assert(sta_netif);
                #ifdef CONFIG_ESP_WIFI_SOFTAP_SUPPORT
                    esp_netif_t *ap_netif = esp_netif_create_default_wifi_ap();
                    assert(ap_netif);
                #endif
                
                // Add event handlers
                ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
                // ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &ip_event_handler, NULL));

                // Start Wi-Fi
                wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
                ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
                ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
                ESP_ERROR_CHECK( esp_wifi_start() );

                /* ESP32-C3 has less power; 34 * 0.25 dBm = 8.5 dBm */
                ESP_ERROR_CHECK( esp_wifi_set_max_tx_power(34) );
                return ESP_OK;
            }

            esp_err_t unload() {
                return ESP_OK;
            }

            static const char* TAG;

            static void connect(void);
            static bool reconnect(void);

            void set_mode(wifi_mode_t mode);
            void set_sta_bssid(const std::string& bssid);
            void set_sta_ssid(const std::string& ssid);
            void set_sta_passwd(const std::string& passwd);

        private:
        
            static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
            static void ip_event_handler(void* arg, esp_event_base_t event_base,int32_t event_id, void* event_data);
    };

}
#if !CONFIG_HTTPD_WS_SUPPORT
    #error This driver requires CONFIG_HTTPD_WS_SUPPORT enabled on menuconfig
#endif

#pragma once

#include "esp_err.h"

#include "system/wifi.hpp"
#include "../quimblos/driver.hpp"

#include <cstdint>
#include <esp_https_server.h>

#if !CONFIG_IDF_TARGET_LINUX
#include <esp_system.h>
#include "esp_wifi.h"
#include "lwip/sockets.h"
#else
#include <unistd.h>
#endif  // !CONFIG_IDF_TARGET_LINUX

namespace driver {

    class WebSocket: public quimblos::Driver {

        public:

        static httpd_handle_t server;

        static struct Config {
            uint8_t max_clients;
        } config;
        
        public:

            WebSocket(WiFi& wifi, uint8_t max_clients = 2) {
                config.max_clients = max_clients;
            }

            esp_err_t load() {

            /* Register event handlers to start server when Wi-Fi or Ethernet is connected,
            * and stop server when disconnection happens.
            */

                #if !CONFIG_IDF_TARGET_LINUX
                /* Has to run before the first connection attempt, so that the TX power and power-save
                * settings below are in effect when the station authenticates with the AP. */
                ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &connect_handler, &server));
                ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &disconnect_handler, &server));
                #endif // !CONFIG_IDF_TARGET_LINUX

                #if CONFIG_IDF_TARGET_LINUX
                    /* On Linux, start the server directly since there are no WiFi/Ethernet events */
                    server = start_wss_echo_server();
                #endif // CONFIG_IDF_TARGET_LINUX

                return ESP_OK;
            }

            esp_err_t unload() {
                return ESP_OK;
            }

            static const char* TAG;

            void send_messages();
            
        private:

            static void connect_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
            static void disconnect_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);

            
    };

}
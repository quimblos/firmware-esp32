#include <unordered_map>
#if !CONFIG_HTTPD_WS_SUPPORT
    #error This driver requires CONFIG_HTTPD_WS_SUPPORT enabled on menuconfig
#endif

#pragma once

#include "esp_err.h"

#include "../system/wifi.hpp"
#include "../../quimblos/engine.hpp"

#include <functional>
#include <cstdint>
#include <esp_https_server.h>

#if !CONFIG_IDF_TARGET_LINUX
#include <esp_system.h>
#include "esp_wifi.h"
#include "lwip/sockets.h"
#else
#include <unistd.h>
#endif  // !CONFIG_IDF_TARGET_LINUX

namespace qb {

    namespace driver {
    
        class WebSocket: public qb::Driver {
    
            typedef const std::function<void(const qb::msg_wrap_t*)> cb_t;
    
            static httpd_handle_t server;
            
            static struct Config {
                std::string uri;
                uint8_t max_clients;
            } config;
            
            inline static std::unordered_map<uint8_t, cb_t> callbacks;
    
            public:
    
                WebSocket(WiFi& wifi, const Config& config = {
                    .uri = "/ws",
                    .max_clients = 2
                }) {
                    WebSocket::config = config;
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
                
                void bind(std::unordered_map<uint8_t, cb_t> callbacks) {
                    WebSocket::callbacks = callbacks;
                }
                
            private:
    
                static void connect_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
                static void disconnect_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
    
                static httpd_handle_t start_wss_server(void);
                static esp_err_t ws_handler(httpd_req_t *req);
    
                static void msg_handler(const std::string& payload);
    
                
        };
    
    }
}
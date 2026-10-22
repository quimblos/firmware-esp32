#pragma once

#include <functional>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "esp_log.h"

#include "driver/usb_serial_jtag.h"
#include <stdio.h>
#include <fcntl.h>

#include "esp_err.h"
#include "../../quimblos/engine.hpp"

namespace qb {

    namespace driver {
    
        class JTAG: public qb::Driver {
    
            typedef const std::function<void(const qb::msg_wrap_t*)> cb_t;            
            inline static std::unordered_map<uint8_t, cb_t> callbacks;

            public:
    
                esp_err_t load() {
                    // 1. Install the driver
                    usb_serial_jtag_driver_config_t config = {
                        .tx_buffer_size = 1024,
                        .rx_buffer_size = 1024,
                    };
                    ESP_ERROR_CHECK(usb_serial_jtag_driver_install(&config));
                    ESP_ERROR_CHECK(create_task());
                    return ESP_OK;
                }
    
                esp_err_t unload() {
                    return ESP_OK;
                }
    
                static const char* TAG;
    
                void bind(std::unordered_map<uint8_t, cb_t> callbacks) {
                    JTAG::callbacks = callbacks;
                }

            private:
                esp_err_t create_task();
                void task();
                void msg_handler(const std::string& payload);
        };
    
    }
    
}
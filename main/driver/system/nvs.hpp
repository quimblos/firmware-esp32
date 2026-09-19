#pragma once

#include "esp_err.h"
#include "nvs_flash.h"

#include "../../quimblos/driver.hpp"

namespace driver {

    class NVS: public quimblos::Driver {

        public:

            esp_err_t load() {
                auto ret = nvs_flash_init();
                if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
                    ESP_ERROR_CHECK(nvs_flash_erase());
                    ret = nvs_flash_init();
                }
                ESP_ERROR_CHECK( ret );
                return ESP_OK;
            }

            esp_err_t unload() {
                return ESP_OK;
            }

            static const char* TAG;

        private:
            
            esp_err_t make_encoder();
    };

}
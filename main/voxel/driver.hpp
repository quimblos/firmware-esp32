#pragma once

#include "data.hpp"
#include "../quimblos/driver.hpp"
#include "driver/ws281x.hpp"
#include "esp_err.h"
#include "soc/gpio_num.h"
#include "impulse.hpp"

namespace voxel {

    class Driver: public quimblos::Driver {

        protected:
        
            driver::WS281x ws281x;
            Grid grid;
            
            
        public:
            
            std::vector<Voxel*> mapping;
            ImpulseAnimator impulses;

        public:

            Driver(gpio_num_t gpio, uint8_t w, uint8_t h):
                ws281x(gpio, w*h),
                grid(w,h),
                impulses(*this) {}

            esp_err_t load() {
                ws281x.load();
                ESP_ERROR_CHECK(make_task());
                return ESP_OK;
            }

            esp_err_t unload() {
                return ESP_OK;
            }

            static const char* TAG;

            esp_err_t map(const std::vector<XY>& coord) {
                mapping.resize(coord.size());
                for (size_t i = 0; i < coord.size(); i++) {
                    size_t j = coord[i].y * grid.w + coord[i].x;
                    if (j >= grid.voxels.size()) {
                        mapping.clear();
                        ESP_LOGW(TAG, "Attempt to map voxel grid failed, voxel #%d (%d, %d) is out of range. Mapping cleared.", i, coord[i].x, coord[i].y);
                        return ESP_FAIL;
                    }
                    mapping[i] = &grid.voxels[j];
                }
                return ESP_OK;
            }

            void test() {
                ws281x.test();
            }

        private:
            
            esp_err_t make_task();
            void task();
    };

}
#pragma once

#include "data.hpp"
#include "../quimblos/engine.hpp"
#include "../quimblos/queue.hpp"
#include "driver/led/ws281x.hpp"
#include "esp_err.h"
#include "soc/gpio_num.h"
#include "impulse.hpp"
#include <cstdint>

namespace voxel {

    class Driver: public qb::Driver {

        friend struct impulse::Animation;
        friend class impulse::Animator;

        protected:

            qb::driver::WS281x ws281x;
            qb::Queue<const qb::msg_wrap_t> queue;

            uint8_t tick_ms;
            Grid grid;
            std::vector<Voxel*> mapping;
            
            impulse::Animator impulses;
            
        public:

            Driver(gpio_num_t gpio, uint8_t w, uint8_t h, uint8_t tick_ms = 10):
                ws281x(gpio, w*h),
                queue(TAG),
                tick_ms(tick_ms),
                grid(w,h),
                impulses(*this) {}

            esp_err_t load() {
                ws281x.load();
                ESP_ERROR_CHECK(create_task());
                return ESP_OK;
            }

            esp_err_t unload() {
                return ESP_OK;
            }

            static const char* TAG;
        
        public:

            esp_err_t map(const std::vector<XY>& coord);
            esp_err_t add_impulse(data::Impulse impulse, const std::vector<uint16_t>& voxels);

            void test() { ws281x.test(); }

        protected:

            void flush();

        private:
            
            esp_err_t create_task();
            void task();
    };

}
#pragma once

#include "data.hpp"
#include "../quimblos/driver.hpp"
#include "../quimblos/queue.hpp"
#include "driver/ws281x.hpp"
#include "esp_err.h"
#include "soc/gpio_num.h"
#include "impulse.hpp"
#include <cstdint>
#include "qb.hpp"

namespace voxel {

    class Driver: public quimblos::Driver {

        protected:
            quimblos::Queue<quimblos::msg_wrap_t> queue;
        
            driver::WS281x ws281x;
            Grid grid;

            uint8_t tick_ms;

            ImpulseAnimator impulses;
            
        public:
            
            std::vector<Voxel*> mapping;

        public:

            Driver(gpio_num_t gpio, uint8_t w, uint8_t h, uint8_t tick_ms = 10):
                queue(TAG),
                ws281x(gpio, w*h),
                grid(w,h),
                tick_ms(tick_ms),
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

            esp_err_t map(const std::vector<XY>& coord);
            void flush();

            esp_err_t add_impulse(Impulse impulse, const std::vector<uint16_t>& voxels) {
                return queue.push(qb.msg.AddImpulse.wrap(
                    new const QB::msg::AddImpulse({
                        .impulse = std::move(impulse),
                        .voxels = std::move(voxels)
                    })
                ));
            }

            void test() {
                ws281x.test();
            }

        private:
            
            esp_err_t make_task();
            void task();
    };

}
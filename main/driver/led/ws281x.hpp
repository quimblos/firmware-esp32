#pragma once

#include <cstdio>
#include <stdint.h>
#include <string>
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/idf_additions.h"
#include "freertos/projdefs.h"
#include "portmacro.h"
#include "soc/gpio_num.h"
#include "driver/rmt_tx.h"
#include "driver/rmt_encoder.h"

#include "../../quimblos/driver.hpp"
#include "util/color.hpp"

namespace qb {

    namespace driver {
    
        class WS281x: public qb::Driver {
    
            public:
    
                struct Encoder {
                    rmt_encoder_t base;
                    rmt_encoder_t *bytes_encoder;
                    rmt_encoder_t *copy_encoder;
                    int state;
                    rmt_symbol_word_t reset_code;
                };
    
            protected:
            
                gpio_num_t gpio;
                uint32_t resolution_hz;
    
                rmt_channel_handle_t chan = NULL;
                rmt_encoder_handle_t encoder = NULL;
    
                uint16_t pixels;
    
                rmt_transmit_config_t tx_config = {
                    .loop_count = 0,
                    .flags = {
                        .eot_level = 0,
                        .queue_nonblocking = 0
                    }
                };
    
            public:
            
                uint8_t* data = nullptr;
    
            public:
    
                WS281x(gpio_num_t gpio, uint16_t pixels, uint32_t resolution_hz = 10000000):
                    gpio(gpio),
                    resolution_hz(resolution_hz),
                    pixels(pixels),
                    data(new uint8_t[pixels*3]()) {}
    
                esp_err_t load() {
    
                    ESP_LOGI(TAG, "Creating RMT TX channel...");
                    rmt_tx_channel_config_t tx_chan_config = {
                        .gpio_num = gpio,
                        .clk_src = RMT_CLK_SRC_DEFAULT, // select source cloc
                        .resolution_hz = resolution_hz,
                        .mem_block_symbols = 64, // increase the block size can make the LED less flickerin
                        .trans_queue_depth = 4, // set the number of transactions that can be pending in the backgroun
                        .intr_priority = 0,
                        .flags = {
                            .invert_out = 0,
                            .with_dma = 0,
                            .allow_pd = 0,
                            .init_level = 0,
                        }
                    };
                    ESP_ERROR_CHECK(rmt_new_tx_channel(&tx_chan_config, &chan));
    
                    ESP_LOGI(WS281x::TAG, "Installing led strip encoder...");
                    ESP_ERROR_CHECK(make_encoder());
    
                    ESP_LOGI(WS281x::TAG, "Enabling RMT TX channel");
                    ESP_ERROR_CHECK(rmt_enable(chan));
    
                    return ESP_OK;
                }
    
                esp_err_t unload() {
                    return ESP_OK;
                }
    
                static const char* TAG;
    
                void test() {
                    uint32_t tick = 0;
    
                    ESP_LOGI(TAG, "Running test pattern on main task...");
                    
                    while (1) {
                        for (uint16_t i = pixels-1; i > 0; i--) {
                            uint16_t p1 = i*3;
                            uint16_t p0 = (i-1)*3;
                            
                            uint16_t v0 = data[p0] * 0xFF;
                            v0 -= v0/10;
    
                            data[p1+0] = data[p0+0];
                            data[p1+1] = data[p0+1];
                            data[p1+2] = data[p0+2];
                        }
                        if (tick > 10) {
                            tick = 0;
                            data[0] = 0xFF;
                            data[1] = 0xFF;
                            data[2] = 0xFF;
                        }
                        else {
                            data[0] = 0x00;
                            data[1] = 0x00;
                            data[2] = 0x00;
                        }
                        tick++;
    
                        // Flush RGB values to LEDs
                        flush();
                        vTaskDelay(pdMS_TO_TICKS(100));
                    }
                }
    
                void set(uint16_t i, const RGB& color) {
                    data[i*3+0] = color.g;
                    data[i*3+1] = color.r;
                    data[i*3+2] = color.b;
                }
    
                void flush() {
    
                    std::string data_str;
                    for (int i = 0; i < pixels*3; i++) {
                        data_str += std::to_string(data[i]) + " ";
                    }
                    ESP_LOGI(TAG, "data: %s", data_str.c_str());
    
                    ESP_ERROR_CHECK(rmt_transmit(chan, encoder, data, pixels*3, &tx_config));
                    ESP_ERROR_CHECK(rmt_tx_wait_all_done(chan, portMAX_DELAY));
                }
    
            private:
                
                esp_err_t make_encoder();
        };
    
    }
}
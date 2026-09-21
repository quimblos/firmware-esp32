/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "esp_check.h"
#include "ws281x.hpp"
using namespace qb::driver;

const char* WS281x::TAG = "WS281x";

RMT_ENCODER_FUNC_ATTR
static size_t rmt_encode_led_strip(rmt_encoder_t *encoder, rmt_channel_handle_t channel, const void *primary_data, size_t data_size, rmt_encode_state_t *ret_state)
{
    WS281x::Encoder *led_encoder = __containerof(encoder, WS281x::Encoder, base);
    rmt_encoder_handle_t bytes_encoder = led_encoder->bytes_encoder;
    rmt_encoder_handle_t copy_encoder = led_encoder->copy_encoder;
    rmt_encode_state_t session_state = RMT_ENCODING_RESET;
    rmt_encode_state_t state = RMT_ENCODING_RESET;
    size_t encoded_symbols = 0;
    switch (led_encoder->state) {
    case 0: // send RGB data
        encoded_symbols += bytes_encoder->encode(bytes_encoder, channel, primary_data, data_size, &session_state);
        if (session_state & RMT_ENCODING_COMPLETE) {
            led_encoder->state = 1; // switch to next state when current encoding session finished
        }
        if (session_state & RMT_ENCODING_MEM_FULL) {
            state = (rmt_encode_state_t)(state | RMT_ENCODING_MEM_FULL);
            goto out; // yield if there's no free space for encoding artifacts
        }
    // fall-through
    case 1: // send reset code
        encoded_symbols += copy_encoder->encode(copy_encoder, channel, &led_encoder->reset_code,
                                                sizeof(led_encoder->reset_code), &session_state);
        if (session_state & RMT_ENCODING_COMPLETE) {
            led_encoder->state = RMT_ENCODING_RESET; // back to the initial encoding session
            state = (rmt_encode_state_t)(state | RMT_ENCODING_COMPLETE);
        }
        if (session_state & RMT_ENCODING_MEM_FULL) {
            state = (rmt_encode_state_t)(state | RMT_ENCODING_MEM_FULL);
            goto out; // yield if there's no free space for encoding artifacts
        }
    }
out:
    *ret_state = state;
    return encoded_symbols;
}

static esp_err_t rmt_del_led_strip_encoder(rmt_encoder_t *encoder)
{
    WS281x::Encoder *led_encoder = __containerof(encoder, WS281x::Encoder, base);
    rmt_del_encoder(led_encoder->bytes_encoder);
    rmt_del_encoder(led_encoder->copy_encoder);
    free(led_encoder);
    return ESP_OK;
}

RMT_ENCODER_FUNC_ATTR
static esp_err_t rmt_led_strip_encoder_reset(rmt_encoder_t *encoder)
{
    WS281x::Encoder *led_encoder = __containerof(encoder, WS281x::Encoder, base);
    rmt_encoder_reset(led_encoder->bytes_encoder);
    rmt_encoder_reset(led_encoder->copy_encoder);
    led_encoder->state = RMT_ENCODING_RESET;
    return ESP_OK;
}

esp_err_t WS281x::make_encoder()
{
    esp_err_t ret = ESP_OK;
    WS281x::Encoder *led_encoder = NULL;
    // C++ forbids a `goto` that jumps over an initialization, so every local is
    // declared (and zero/default-initialized) before the first ESP_GOTO_* macro.
    rmt_bytes_encoder_config_t bytes_encoder_config = {};
    rmt_copy_encoder_config_t copy_encoder_config = {};
    uint32_t reset_ticks = 0;
    led_encoder = static_cast<WS281x::Encoder *>(rmt_alloc_encoder_mem(sizeof(WS281x::Encoder)));
    ESP_GOTO_ON_FALSE(led_encoder, ESP_ERR_NO_MEM, err, TAG, "no mem for led strip encoder");
    led_encoder->base.encode = rmt_encode_led_strip;
    led_encoder->base.del = rmt_del_led_strip_encoder;
    led_encoder->base.reset = rmt_led_strip_encoder_reset;
    // different led strip might have its own timing requirements, following parameter is for WS2812
    // (designated initializers are C-only; set the members explicitly instead)
    bytes_encoder_config.bit0.level0 = 1;
    bytes_encoder_config.bit0.duration0 = 0.3 * resolution_hz / 1000000; // T0H=0.3us
    bytes_encoder_config.bit0.level1 = 0;
    bytes_encoder_config.bit0.duration1 = 0.9 * resolution_hz / 1000000; // T0L=0.9us
    bytes_encoder_config.bit1.level0 = 1;
    bytes_encoder_config.bit1.duration0 = 0.9 * resolution_hz / 1000000; // T1H=0.9us
    bytes_encoder_config.bit1.level1 = 0;
    bytes_encoder_config.bit1.duration1 = 0.3 * resolution_hz / 1000000; // T1L=0.3us
    bytes_encoder_config.flags.msb_first = 1; // WS2812 transfer bit order: G7...G0R7...R0B7...B0
    ESP_GOTO_ON_ERROR(rmt_new_bytes_encoder(&bytes_encoder_config, &led_encoder->bytes_encoder), err, TAG, "create bytes encoder failed");
    ESP_GOTO_ON_ERROR(rmt_new_copy_encoder(&copy_encoder_config, &led_encoder->copy_encoder), err, TAG, "create copy encoder failed");

    reset_ticks = resolution_hz / 1000000 * 50 / 2; // reset code duration defaults to 50us
    led_encoder->reset_code.level0 = 0;
    led_encoder->reset_code.duration0 = reset_ticks;
    led_encoder->reset_code.level1 = 0;
    led_encoder->reset_code.duration1 = reset_ticks;
    this->encoder = &led_encoder->base;
    return ESP_OK;
err:
    if (led_encoder) {
        if (led_encoder->bytes_encoder) {
            rmt_del_encoder(led_encoder->bytes_encoder);
        }
        if (led_encoder->copy_encoder) {
            rmt_del_encoder(led_encoder->copy_encoder);
        }
        free(led_encoder);
    }
    return ret;
}

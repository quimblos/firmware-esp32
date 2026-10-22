#include "jtag.hpp"
using namespace qb::driver;

const char* JTAG::TAG = "JTAG";

constexpr size_t BUFFER_SIZE = 4096;

esp_err_t JTAG::create_task() {
    auto ret = xTaskCreate(
        [](void* driver) {
            ((JTAG* ) driver)->task();
        },
        "jtag",
        3072,
        this,
        5,
        NULL
    );
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create the task");
        return ret;
    }
    return ESP_OK;
}

void JTAG::task() {
    static char frame[BUFFER_SIZE];

    while (true) {
        char byte;
        if (usb_serial_jtag_read_bytes(&byte, 1, portMAX_DELAY) != 1) {
            continue;
        }
        if (byte != '{') {
            continue;
        }

        size_t len = 0;
        frame[len++] = byte;

        bool terminated = false;
        bool overflow = false;
        while (true) {
            if (usb_serial_jtag_read_bytes(&byte, 1, pdMS_TO_TICKS(100)) != 1) {
                break;
            }
            if (byte == '\0') {
                terminated = true;
                break;
            }
            if (len + 1 >= BUFFER_SIZE) {
                overflow = true;
                continue;
            }
            frame[len++] = byte;
        }

        if (overflow) {
            ESP_LOGE(TAG, "Dropping serial input longer than %u bytes", (unsigned) (BUFFER_SIZE - 1));
            continue;
        }
        if (!terminated) {
            ESP_LOGW(TAG, "Dropping truncated serial input (%u bytes)", (unsigned) len);
            continue;
        }

        frame[len] = '\0';
        msg_handler(std::string(frame, len));
    }
}

void JTAG::msg_handler(const std::string& payload) {
    const JSON json = JSON::parse(payload);
    const JSON& _kind = json.get("$");
    if (_kind.is_empty()) {
        ESP_LOGW(TAG, "Ignoring message without a '$' kind: %s", payload.c_str());
        return;
    }
    auto kind = _kind.as<uint8_t>();

    if (callbacks.contains(kind)) {
        ESP_LOGI(TAG, "Parsing message of kind %d", kind);
        auto wrap = engine->wrap_from_json(kind, json);
        ESP_LOGI(TAG, "%s", engine->unwrap_to_json(wrap).c_str());
        callbacks.at(kind)(wrap);
        delete wrap;
    }
    else {
        ESP_LOGW(TAG, "Unknown message kind %d", kind);
    }
}
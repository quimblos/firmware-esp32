#include "driver.hpp"
#include "esp_err.h"
using namespace voxel;

const char* Driver::TAG = "Voxel";

esp_err_t Driver::map(const std::vector<XY>& coord) {
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

void Driver::flush() {
    for (uint16_t i = 0; i < mapping.size(); i++) {
        ws281x.set(i, mapping[i]->data);
    }
    ws281x.flush();
}

esp_err_t Driver::make_task() {
    auto ret = xTaskCreate(
        [](void* driver) {
            ((Driver* ) driver)->task();
        },
        "voxel",
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

void Driver::task() {
    while (1) {
        queue.wait([this](Msg& msg) {
            switch (msg.type) {
                case Msg::ADD_IMPULSE:
                    impulses.add(msg.data.add_impulse->impulse, msg.data.add_impulse->voxels); break;
            }
        });
        impulses.tick();
        vTaskDelay(tick_ms / portTICK_PERIOD_MS);
    }
}
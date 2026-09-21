#include "driver.hpp"
#include "esp_err.h"

#include "qb.hpp"
#include "quimblos/data.hpp"
#include "voxel/impulse.hpp"
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

esp_err_t Driver::add_impulse(data::Impulse impulse, const std::vector<uint16_t>& voxels) {
    ASSERT(
        "Impulse signal must have length between 1 and 256",
        impulse.signal.size() > 0 && impulse.signal.size() <= 256
    )
    ASSERT(
        "Voxels must have length between 1 and 0xFFFF",
        voxels.size() > 0 && voxels.size() <= 0xFFFF
    )

    // auto msg = new const voxel::msg::AddImpulse(std::move(impulse), std::move(voxels));
    // return queue.push(msg->wrap());
    return ESP_OK;
}

/* Tasks */

esp_err_t Driver::create_task() {
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
        queue.wait([this](const qb::msg_wrap_t& wrap) {
            switch (wrap.kind) {
                case voxel::msg_t::AddImpulse: {
                    auto& msg = voxel::msg::AddImpulse::unwrap(wrap);
                    impulses.add(msg.impulse, msg.voxels);
                    break;
                }
            }
        });
        impulses.tick();
        vTaskDelay(tick_ms / portTICK_PERIOD_MS);
    }
}
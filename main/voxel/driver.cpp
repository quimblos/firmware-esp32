#include "driver.hpp"
#include "esp_err.h"

#include "qb.hpp"
#include "quimblos/data.hpp"
#include "voxel/impulse.hpp"
using namespace voxel;

const char* Driver::TAG = "Voxel";

/* API */

esp_err_t Driver::set_grid(uint8_t w, uint8_t h) {
    ASSERT(
        "Grid width must be > 0",
        w > 0
    )
    ASSERT(
        "Grid height must be > 0",
        h > 0
    )

    auto msg = new const voxel::msg::SetGrid({
        .w = std::move(w),
        .h = std::move(h)
    });
    return queue.push(msg->wrap());
    return ESP_OK;
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

    auto msg = new const voxel::msg::AddImpulse({
        .impulse = std::move(impulse),
        .voxels = std::move(voxels)
    });
    return queue.push(msg->wrap());
    return ESP_OK;
}

/* Internal API */

void Driver::flush() {
    for (uint16_t i = 0; i < grid.voxels.size(); i++) {
        ws281x.set(i, grid.voxels[i].data);
    }
    ws281x.flush();
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
                case voxel::msg_t::SetGrid: {
                    auto& msg = voxel::msg::SetGrid::unwrap(wrap);
                    grid = Grid(msg.w, msg.h);
                    ws281x.resize(msg.w*msg.h);
                    break;
                }
                case voxel::msg_t::AddImpulse: {
                    auto& msg = voxel::msg::AddImpulse::unwrap(wrap);
                    impulses.add(msg.impulse, msg.voxels);
                    break;
                }
            }
        });
        impulses.tick();
        flush();
        vTaskDelay(tick_ms / portTICK_PERIOD_MS);
    }
}
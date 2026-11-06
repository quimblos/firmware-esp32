#include "impulse.hpp"
#include "driver.hpp"
#include <algorithm>
#include <cstdint>
#include <new>
#include <utility>
using namespace voxel;

bool impulse::Animation::tick(Driver& driver) {

    // Interpolate signal frames
    uint8_t frame = 0;
    if (t < impulse.signal.size() - 1) {
        float r = (float) ti / impulse.signal[t].dur;
        frame = impulse.signal[t].val*(1-r) + impulse.signal[t+1].val*r;
    }
    else if (t == impulse.signal.size() - 1) {
        frame = impulse.signal[t].val;
    }

    // Update channel data
    for (const auto index: voxels) {
        switch (impulse.channel) {
            case data::ImpulseChannel::R:
                driver.grid.voxels[index].data.r = frame; break;
            case data::ImpulseChannel::G:
                driver.grid.voxels[index].data.g = frame; break;
            case data::ImpulseChannel::B:
                driver.grid.voxels[index].data.b = frame; break;
        }
    }

    // Advance animation
    ti += driver.tick_ms;
    if (ti >= impulse.signal[t].dur) {
        ti = 0;
        t++;
    }

    // Animation end
    if (t >= impulse.signal.size()) {
        clear(driver);
        return true;
    }
    return false;
}

void impulse::Animation::clear(Driver& driver) {
    for (const auto index: voxels) {
        switch (impulse.channel) {
            case data::ImpulseChannel::R:
                driver.grid.voxels[index].data.r = 0; break;
            case data::ImpulseChannel::G:
                driver.grid.voxels[index].data.g = 0; break;
            case data::ImpulseChannel::B:
                driver.grid.voxels[index].data.b = 0; break;
        }
    }
}

esp_err_t impulse::Animator::add(data::Impulse impulse, const std::vector<uint16_t>& voxels) {
    
    for (const auto& index: voxels) {
        if (index >= driver.grid.w*driver.grid.h) {
            ESP_LOGW(Driver::TAG, "Attempt to add impulse animation failed, voxel #%d is out of range. impulse::Animation not added.", index);
            return ESP_FAIL;
        }
    }

    // Add animation
    queue.push_back(impulse::Animation(impulse, voxels));
    return ESP_OK;
}

void impulse::Animator::tick() {
    if (queue.size() == 0) return;
    ESP_LOGD(Driver::TAG, "impulse::Animator tick, queue: %zu", queue.size());
    std::vector<impulse::Animation>::iterator it = queue.begin();
    if (it->tick(driver)) {
        queue.erase(it);
        if (queue.size() == 0) driver.clear();
    }
}

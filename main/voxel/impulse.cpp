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
            case data::Impulse::R:
                driver.mapping[index]->data.r = frame; break;
            case data::Impulse::G:
                driver.mapping[index]->data.g = frame; break;
            case data::Impulse::B:
                driver.mapping[index]->data.b = frame; break;
        }
    }

    // Advance animation
    ti++;
    if (ti >= impulse.signal[t].dur) {
        ti = 0;
        t++;
    }

    // Animation end
    if (t >= impulse.signal.size()) {
        clear(driver);
        // DEBUG: repeat impulse
        driver.add_impulse(impulse, voxels);
        return true;
    }
    return false;
}

void impulse::Animation::clear(Driver& driver) {
    for (const auto index: voxels) {
        switch (impulse.channel) {
            case data::Impulse::R:
                driver.mapping[index]->data.r = 0; break;
            case data::Impulse::G:
                driver.mapping[index]->data.g = 0; break;
            case data::Impulse::B:
                driver.mapping[index]->data.b = 0; break;
        }
    }
}

esp_err_t impulse::Animator::add(data::Impulse impulse, const std::vector<uint16_t>& voxels) {
    
    for (const auto& index: voxels) {
        if (index >= driver.mapping.size()) {
            ESP_LOGW(Driver::TAG, "Attempt to add impulse animation failed, voxel #%d is out of range. impulse::Animation not added.", index);
            return ESP_FAIL;
        }
    }

    // Remove voxels from previous animations if they're used on the new one
    auto indexes = std::vector<uint16_t>(voxels);
    std::sort(indexes.begin(), indexes.end());
    for (auto& anim: animations) {
        if (anim.impulse.channel != impulse.channel) continue;
        anim.voxels.erase(
            std::remove_if(anim.voxels.begin(), anim.voxels.end(),
                [&indexes](int x) { return std::binary_search(indexes.begin(), indexes.end(), x); }),
            anim.voxels.end()
        );
    }

    // Add animation
    animations.push_back(impulse::Animation(impulse, voxels));
    return ESP_OK;
}

void impulse::Animator::tick() {
    ESP_LOGD(Driver::TAG, "impulse::Animator tick, animations: %zu", animations.size());
    std::vector<impulse::Animation>::iterator it = animations.begin();
    while(it != animations.end()) {
        if(it->tick(driver)) {
            it = animations.erase(it);
        }
        else ++it;
    }

    driver.flush();
}

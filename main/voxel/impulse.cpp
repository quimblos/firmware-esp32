#include "impulse.hpp"
#include "driver.hpp"
#include <cstdint>
using namespace voxel;

bool Animation::tick(Driver& driver) {
    auto frame = impulse.wave[t];
    for (const auto index: voxels) {
        switch (impulse.channel) {
            case Impulse::R:
                driver.mapping[index]->data.r = frame; break;
            case Impulse::G:
                driver.mapping[index]->data.g = frame; break;
            case Impulse::B:
                driver.mapping[index]->data.b = frame; break;
        }
    }
    t++;
    return t >= impulse.wave.size();
}

esp_err_t ImpulseAnimator::add(Impulse impulse, const std::vector<uint16_t>& voxels) {
    for (const auto& index: voxels) {
        if (index >= driver.mapping.size()) {
            ESP_LOGW(Driver::TAG, "Attempt to add impulse animation failed, voxel #%d is out of range. Animation not added.", index);
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
    animations.push_back(Animation(impulse, voxels));
    return ESP_OK;
}

void ImpulseAnimator::tick() {
    ESP_LOGI(Driver::TAG, "ImpulseAnimator tick, animations: %d", animations.size());
    std::vector<Animation>::iterator it = animations.begin();
    while(it != animations.end()) {
        if(it->tick(driver)) {
            it = animations.erase(it);
        }
        else ++it;
    }
}
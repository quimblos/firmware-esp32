#pragma once

#include <algorithm>
#include <cstdint>
#include <vector>

#include "esp_err.h"

namespace voxel {

    class Driver;

    struct Impulse {
        enum Channel {
            R,
            G,
            B
        } channel;
        uint16_t framelen;
        std::vector<uint8_t> wave;
    };

    struct Animation {
        Impulse impulse;
        std::vector<uint16_t> voxels;
        uint8_t t = 0;
        
        Animation(Impulse& impulse, const std::vector<uint16_t>& voxels):
            impulse(impulse),
            voxels(voxels) {}

        // Returns true if done
        bool tick(Driver& driver);
    };

    class ImpulseAnimator {
        
        Driver& driver;
        std::vector<Animation> animations;

        public:

            ImpulseAnimator(Driver& driver):
                driver(driver) {}

            esp_err_t add(Impulse impulse, const std::vector<uint16_t>& voxels);
            void tick();

    };

}
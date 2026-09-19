#pragma once

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
        struct Frame {
            uint8_t val;
            uint8_t dur;
        };
        std::vector<Frame> signal;
    };

    struct ImpulseAnimation {
        Impulse impulse;
        std::vector<uint16_t> voxels;
        uint8_t t = 0;
        uint8_t ti = 0;
        
        ImpulseAnimation(Impulse& impulse, const std::vector<uint16_t>& voxels):
            impulse(impulse),
            voxels(voxels) {}

        // Returns true if done
        bool tick(Driver& driver);
        void clear(Driver& driver);
    };

    class ImpulseAnimator {
        Driver& driver;
        std::vector<ImpulseAnimation> animations;

        public:

            ImpulseAnimator(Driver& driver):
                driver(driver) {}

            esp_err_t add(Impulse impulse, const std::vector<uint16_t>& voxels);
            void tick();

    };

}

#pragma once

#include <cstdint>
#include <vector>

#include "esp_err.h"
#include "quimblos/engine.hpp"

QB_DATA(voxel,
    QB_OBJ(Frame, (
        (val, uint8_t),
        (dur, uint8_t)
    )),
    QB_VEC(data::Frame),
    QB_OBJ(Impulse, (
        (channel, enum Channel { R, G, B }),
        (signal, std::vector<Frame>)
    ))
)

namespace voxel {


    class Driver;


    namespace impulse {

        struct Animation {
            data::Impulse impulse;
            std::vector<uint16_t> voxels;
            uint8_t t = 0;
            uint8_t ti = 0;
            
            Animation(data::Impulse& impulse, const std::vector<uint16_t>& voxels):
                impulse(impulse),
                voxels(voxels) {}
    
            // Returns true if done
            bool tick(Driver& driver);
            void clear(Driver& driver);
        };
    
        class Animator {
            Driver& driver;
            std::vector<Animation> animations;
    
            public:
    
                Animator(Driver& driver):
                    driver(driver) {}
    
                esp_err_t add(data::Impulse impulse, const std::vector<uint16_t>& voxels);
                void tick();
    
        };
    }

}

#pragma once

#include <cstdint>
#include <vector>

#include "esp_err.h"
#include "quimblos/data.hpp"

QB_DATA(voxel,
    QB_OBJ(Frame, (
        (val, uint8_t),
        (dur, uint8_t)
    )),
    QB_ENUM(ImpulseChannel, ( R, G, B )),
    QB_VEC(data::Frame),
    QB_OBJ(Impulse, (
        (channel, data::ImpulseChannel),
        (signal, std::vector<data::Frame>)
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

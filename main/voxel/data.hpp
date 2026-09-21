#pragma once

#include <cstdint>
#include <sstream>
#include <vector>

#include "util/color.hpp"
#include "quimblos/engine.hpp"

QB_DATA(voxel,
    QB_VEC(uint8_t)
)

namespace voxel {

    struct XY {
        uint8_t x;
        uint8_t y;
    };

    struct Voxel {
        uint16_t index; // 0xFFFF -> unassigned

        RGB data;

        Voxel(uint16_t index = 0xFFFF):
            index(index) {}
        
        void set(uint8_t r, uint8_t g, uint8_t b) {
            data.r = r;
            data.g = g;
            data.b = b;
        }
    };

    struct Grid {

        uint8_t w;
        uint8_t h;
        std::vector<Voxel> voxels;

        Grid(uint8_t w, uint8_t h):
            w(w),
            h(h),
            voxels(std::vector<Voxel>(w*h)) {}

    };


}
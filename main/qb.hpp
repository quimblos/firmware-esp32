#pragma once

#include "quimblos/engine.hpp"

#include "driver/system/nvs.hpp"
#include "driver/system/jtag.hpp"

#include "quimblos/macro/engine.h"
#include "voxel/driver.hpp"

#define VOXEL_GPIO      4
#define VOXEL_GRID_W    3
#define VOXEL_GRID_H    3

QB_DATA(voxel,
    QB_VEC(uint8_t),
    QB_VEC(uint16_t)
)

QB_ENGINE(voxel,
    QB_DRIVERS(
        (qb::driver::NVS, nvs, ()),
        (qb::driver::JTAG, jtag, ()),
        (voxel::Driver, voxel, ((gpio_num_t) VOXEL_GPIO, VOXEL_GRID_W, VOXEL_GRID_H))
    ),
    QB_MSG(SetGrid, (
        (w, uint8_t),
        (h, uint8_t)
    )),
    QB_MSG(AddImpulse, (
        (impulse, data::Impulse),
        (voxels, std::vector<uint16_t>)
    ))
)
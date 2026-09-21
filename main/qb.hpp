#pragma once

#include "quimblos/engine.hpp"

#include "driver/system/nvs.hpp"
#include "driver/system/wifi.hpp"
#include "driver/system/blufi.hpp"
#include "driver/websocket/ws.hpp"

#include "quimblos/macro/engine.h"
#include "voxel/driver.hpp"

#define VOXEL_GPIO      4
#define VOXEL_GRID_W    3
#define VOXEL_GRID_H    3

QB_ENGINE(voxel,
    QB_DRIVERS(
        (qb::driver::NVS, nvs, ()),
        (qb::driver::WiFi, wifi, (nvs)),
        (qb::driver::BluFi, blufi, (wifi)),
        (qb::driver::WebSocket, websocket, (wifi)),
        (voxel::Driver, voxel, ((gpio_num_t) VOXEL_GPIO, VOXEL_GRID_W, VOXEL_GRID_H))
    ),
    QB_MSG(AddImpulse, (
        (impulse, data::Impulse),
        (voxels, std::vector<uint16_t>)
    ),
        {qb::serial::Prop::UINT8, (void*) &impulse.channel}
    )
)
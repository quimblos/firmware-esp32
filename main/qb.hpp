#pragma once

#include "quimblos/engine.hpp"
#include "quimblos/macro/engine.h"

#include "driver/system/nvs.hpp"
#include "driver/system/wifi.hpp"
#include "driver/system/blufi.hpp"
#include "driver/websocket.hpp"

#include "voxel/driver.hpp"
#include "voxel/impulse.hpp"

#define VOXEL_GPIO      4
#define VOXEL_GRID_W    3
#define VOXEL_GRID_H    3

QB_ENGINE(
    
    QB_DRIVERS(
        (driver::NVS, nvs, ()),
        (driver::WiFi, wifi, (nvs)),
        (driver::BluFi, blufi, (wifi)),
        (driver::WebSocket, websocket, (wifi)),
        (voxel::Driver, voxel, ((gpio_num_t) VOXEL_GPIO, VOXEL_GRID_W, VOXEL_GRID_H))
    ),
    
    QB_MSG(AddImpulse, (
        voxel::Impulse impulse;
        std::vector<uint16_t> voxels;
    ),
        {quimblos::serial::Prop::UINT8, &msg->impulse.channel}
    )
)
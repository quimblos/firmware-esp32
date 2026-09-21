#include "qb.hpp"
#include "voxel/impulse.hpp"
#include <concepts>
#include <cstdint>
#include <initializer_list>

using namespace voxel;
static Engine engine;

extern "C" void app_main(void)
{
    /* WebSocket Routes */
    driver::websocket.bind({
        {msg_t::SetGrid, [](const qb::msg_wrap_t* d) {
            auto& msg = msg::SetGrid::unwrap(*d);
            driver::voxel.set_grid(msg.w, msg.h);
        }},
        {msg_t::MapGrid, [](const qb::msg_wrap_t* d) {
            auto& msg = msg::MapGrid::unwrap(*d);
            driver::voxel.map_grid(msg.coords);
        }},
        {msg_t::AddImpulse, [](const qb::msg_wrap_t* d) {
            auto& msg = msg::AddImpulse::unwrap(*d);
            driver::voxel.add_impulse(msg.impulse, msg.voxels);
        }}
    });

    /* Boot */
    engine.boot();
    
    /* 
        [Wi-Fi]
        If you added the Wi-Fi driver, you can either use BluFi to configure it via Bluetooth,
        or add the credentials manually below.
    */
    // wifi.set_mode(wifi_mode_t::WIFI_MODE_STA);
    // wifi.set_sta_ssid("VIVOFIBA-2871");
    // wifi.set_sta_password("f4AC5S3piJ");
    // wifi.connect();

    /*
        Default mapping
    */

    driver::voxel.map_grid({
        0,0, 0,1, 0,2,
        1,0, 1,1, 1,2,
        2,0, 2,1, 2,2
    });
}

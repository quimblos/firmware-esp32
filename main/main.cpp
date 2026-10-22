#include "qb.hpp"
#include "voxel/impulse.hpp"
#include <initializer_list>

using namespace voxel;
static Engine engine;

extern "C" void app_main(void)
{
    /* JTAG Routes */
    driver::jtag.bind({
        {msg_t::SetGrid, [](const qb::msg_wrap_t* d) {
            auto& msg = msg::SetGrid::unwrap(*d);
            driver::voxel.set_grid(msg.w, msg.h);
        }},
        {msg_t::AddImpulse, [](const qb::msg_wrap_t* d) {
            auto& msg = msg::AddImpulse::unwrap(*d);
            driver::voxel.add_impulse(msg.impulse, msg.voxels);
        }}
    });

    /* Boot */
    engine.boot();
}

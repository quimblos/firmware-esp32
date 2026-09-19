#include "quimblos/engine.hpp"

#include "driver/system/nvs.hpp"
#include "driver/system/wifi.hpp"
#include "driver/system/blufi.hpp"
#include "driver/websocket.hpp"

#include "voxel/driver.hpp"
#include "voxel/impulse.hpp"

#define VOXEL_GPIO      4
#define VOXEL_GRID_W    3
#define VOXEL_GRID_H    3

static const char *TAG = "example";

extern "C" void app_main(void)
{
    /* Drivers
       Static storage duration: returning from app_main() deletes the main task and
       destroys its stack frame, but background tasks and callbacks (the "voxel"
       task, Wi-Fi/BLE handlers, the web server) keep pointers to these objects. */

    static auto nvs = driver::NVS();
    static auto wifi = driver::WiFi(nvs);
    static auto blufi = driver::BluFi(wifi);
    static auto websocket = driver::WebSocket(wifi);
    
    static auto voxel = voxel::Driver((gpio_num_t) VOXEL_GPIO, VOXEL_GRID_W, VOXEL_GRID_H);

    /* Engine */

    auto engine = quimblos::Engine({
        &nvs,
        &wifi,
        &blufi,
        &websocket,
        &voxel
    });
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
        [Voxel]
    */

    voxel.map({
        {0,0},{0,1},{0,2},
        {1,0},{1,1},{1,2},
        {2,0},{2,1},{2,2},
    });

    voxel.add_impulse({
        .channel = voxel::Impulse::R,
        .signal = {
            {.val=0xFF,.dur=200},
            {.val=0x0,.dur=0}
        }
    },
        {0,1,2,3,4,5,6,7,8}
    );
}

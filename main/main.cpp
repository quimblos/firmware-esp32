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
    /* Drivers */

    auto nvs = driver::NVS();
    auto wifi = driver::WiFi(nvs);
    auto blufi = driver::BluFi(wifi);
    auto websocket = driver::WebSocket(wifi);
    
    auto voxel = voxel::Driver((gpio_num_t) VOXEL_GPIO, VOXEL_GRID_W, VOXEL_GRID_H);

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

    voxel.impulses.add({
        .channel = voxel::Impulse::R,
        .framelen = 100,
        .wave = {0xFF,0xEF,0xDF,0xCF,0xBF,0xAF,0x9F,0x8F,0x7F,0x6F,0x5F,0x4F,0x3F,0x2F,0x1F,0x0F}
    },
        {0,1,2,3,4}
    );
}

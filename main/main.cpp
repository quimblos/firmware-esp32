#include "quimblos/engine.hpp"
#include "driver/system/nvs.hpp"
#include "driver/system/wifi.hpp"
#include "driver/system/blufi.hpp"
#include "driver/websocket.hpp"
#include "driver/ws281x.hpp"

#define WS281X_GPIO     4
#define WS281X_PIXELS   9

static const char *TAG = "example";

extern "C" void app_main(void)
{
    /* Drivers */

    auto nvs = driver::NVS();
    auto wifi = driver::WiFi(nvs);
    auto blufi = driver::BluFi(wifi);
    auto websocket = driver::WebSocket(wifi);
    auto ws281x = driver::WS281x((gpio_num_t) WS281X_GPIO, WS281X_PIXELS);

    /* Engine */

    auto engine = quimblos::Engine({
        &nvs,
        &wifi,
        &blufi,
        &websocket,
        &ws281x
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
        [WebSocket]
    */

    // websocket.send_messages();
    ws281x.test();
}

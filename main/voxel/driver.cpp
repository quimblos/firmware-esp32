#include "driver.hpp"
#include "esp_err.h"
using namespace voxel;

const char* Driver::TAG = "Voxel";

esp_err_t Driver::make_task() {
    auto ret = xTaskCreate(
        [](void* driver) {
            ((Driver* ) driver)->task();
        },
        "voxel",
        3072,
        this,
        5,
        NULL
    );
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create the task");
        return ret;
    }
    return ESP_OK;
}

void Driver::task() {
    while (1) {
        impulses.tick();
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
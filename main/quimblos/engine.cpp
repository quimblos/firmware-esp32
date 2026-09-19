#include "esp_log.h"
#include "esp_system.h"
#include "esp_heap_caps.h"

#include "engine.hpp"
using namespace quimblos;

const char* Engine::TAG = "qb.engine";

void Engine::boot() {
    ESP_LOGI(TAG, "Quimblos Engine starting...");

    for (auto& driver: drivers) {
        driver->load();
    } 

    ESP_LOGI(TAG, "[heap] ree=%u min=%u",
         (unsigned)esp_get_free_heap_size(),
         (unsigned)heap_caps_get_minimum_free_size(MALLOC_CAP_8BIT));
}
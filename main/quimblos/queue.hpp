#pragma once

#include <functional>
#include <string>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"

namespace qb {

    template <class T, std::size_t N = 8>
    class Queue {

        const std::string tag;

        std::array<T*, N> storage;
        StaticQueue_t buffer;
        QueueHandle_t handler;

        public:
            Queue(const std::string& tag):
                tag("queue."+tag),
                handler(xQueueCreateStatic(N, sizeof(T*), (uint8_t*) storage.data(), &buffer)) {}

            esp_err_t push(const T* msg) {
                if (msg == nullptr) return ESP_FAIL;
                if (xQueueSend(handler, &msg, 0) != pdTRUE) {
                    ESP_LOGW(tag.c_str(), "Push failed");
                    delete msg;
                    return ESP_FAIL;
                }
                return ESP_OK;
            }

            void wait(std::function<void(T&)> fn) {
                T* msg;
                while(xQueueReceive(handler, &msg, 0) == pdTRUE) {
                    fn(*msg);
                    delete msg;
                }          
            }

    };

}
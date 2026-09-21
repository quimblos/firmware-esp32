#pragma once

#include "esp_err.h"

namespace qb {

    class Engine;

    class Driver {
        public:
            inline static const Engine* engine = nullptr;

        public:
            ~Driver() {}

            virtual esp_err_t load() = 0;
            virtual esp_err_t unload() = 0;
    };

}
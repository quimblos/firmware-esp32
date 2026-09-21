#pragma once

#include "esp_err.h"

namespace qb {

    class Engine;

    class Driver {
        public:
            static const Engine* engine;

        public:
            ~Driver() {}

            virtual esp_err_t load() = 0;
            virtual esp_err_t unload() = 0;
    };

}
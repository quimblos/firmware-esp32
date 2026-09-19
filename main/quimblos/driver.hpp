#pragma once

#include "esp_err.h"

namespace quimblos {

    class Driver {
    
        public:
            ~Driver() {}

            virtual esp_err_t load() = 0;
            virtual esp_err_t unload() = 0;
    
    };

}
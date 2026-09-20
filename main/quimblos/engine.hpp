#pragma once

#include <vector>
#include "driver.hpp"
#include "serial.hpp"

namespace quimblos {

    struct msg_wrap_t {
        uint8_t kind;
        void* data;
    };

    class Engine {
        protected:
            std::vector<Driver*> drivers;

        public:
            Engine(std::vector<Driver*> drivers):
                drivers(drivers) {}

            void boot();

            static const char* TAG;
    };

}
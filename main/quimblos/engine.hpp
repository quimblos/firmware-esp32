#pragma once

#include <vector>
#include "driver.hpp"

namespace quimblos {

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
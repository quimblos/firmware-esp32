#pragma once

#include <cstdint>
#include <string>

namespace qb {
    
    struct msg_wrap_t {
        const uint8_t kind;
        const void* data;
    };

    class Engine {
        public:
            virtual void boot() const;
            virtual const msg_wrap_t* parse(uint8_t kind, const std::string& payload) const;
    };

}

#include "serial.hpp"
#include "driver.hpp"
#include "quimblos/macro/engine.h"
#pragma once

#include <cstdint>
#include <string>

#define ASSERT(ERR, X...) if (!(X)) { ESP_LOGW(TAG, ERR); return ESP_FAIL; }

namespace qb {
    
    struct msg_wrap_t {
        const uint8_t kind;
        const void* data;
    };

    class Engine {
        public:
            virtual void boot() const;
            virtual const std::string unwrap_json(const qb::msg_wrap_t* wrap) const;
            virtual const msg_wrap_t* parse(uint8_t kind, const std::string& payload) const;
    };

    template <class T>
    std::ostream& to_json(std::ostream& os, const T& v) {
        os << v;
        return os;
    }
}

#include "data.hpp"
#include "serial.hpp"
#include "driver.hpp"
#include "quimblos/macro/engine.h"
#pragma once

#include "util/json.hpp"
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
            virtual const std::string unwrap_to_json(const qb::msg_wrap_t* wrap) const;
            virtual const msg_wrap_t* wrap_from_json(uint8_t kind, const JSON& json) const;
    };

}

#include "data.hpp"
#include "driver.hpp"
#include "quimblos/macro/engine.h"
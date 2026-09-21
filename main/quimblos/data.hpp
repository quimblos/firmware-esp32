#pragma once

#include <cstdint>
#include <ostream>
#include <unordered_map>
#include "util/json.hpp"

namespace qb {
    template <class T>
    inline std::ostream& to_json(std::ostream& os, const T& v) {
        os << v;
        return os;
    }

    template <class T>
    inline T from_json(const JSON& json) { return JSON::empty; }

    template<> inline uint8_t from_json(const JSON& json) { return std::atoi(json.value.c_str()); }
    template<> inline uint16_t from_json(const JSON& json) { return std::atoi(json.value.c_str()); }

    inline JSON parse_json(const std::string& payload) {
        return JSON::parse(payload);
    }
}

#include "quimblos/macro/engine.h"
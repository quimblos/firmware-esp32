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
    inline T* from_json(const JSON& json) { return JSON::empty; }

    template<> inline uint8_t* from_json(const JSON& json) { return new uint8_t(std::stoi(json.value)); }
    template<> inline uint16_t* from_json(const JSON& json) { return new uint16_t(std::stoi(json.value)); }

    inline JSON parse_json(const std::string& payload) {
        return JSON::parse(payload);
    }
}

#include "quimblos/macro/engine.h"
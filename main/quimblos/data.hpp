#pragma once

#include <cstdint>
#include <cstdlib>
#include <ostream>
#include "util/json.hpp"

namespace qb {
    template <class T>
    inline std::ostream& to_json(std::ostream& os, const T& v) {
        os << v;
        return os;
    }

    template <class T>
    inline T* from_json(const JSON& json) { return JSON::empty; }

    template<> inline uint8_t* from_json(const JSON& json) { return new uint8_t(json.as<uint8_t>()); }
    template<> inline uint16_t* from_json(const JSON& json) { return new uint16_t(json.as<uint16_t>()); }

    template<> inline std::ostream& to_json<uint8_t>(std::ostream& os, const uint8_t& v) { os << (uint16_t) v; return os; }
    template<> inline std::ostream& to_json<int8_t>(std::ostream& os, const int8_t& v) { os << (int16_t) v; return os; }

    inline JSON parse_json(const std::string& payload) {
        return JSON::parse(payload);
    }
}

#include "quimblos/macro/engine.h"
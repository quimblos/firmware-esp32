#pragma once

#include "deparen.h"
#include "map.h"

#define _ARG0(X, ...) X
#define ARG0(X) _ARG0(X)

#define _ARG1(X, ...) __VA_ARGS__
#define ARG1(X) _ARG1(X)

#define _ENUM(ITEM) ARG0(DEPAREN(ITEM))
#define _STRUCT(ITEM) ARG1(DEPAREN(ITEM))
#define _PARSE(ITEM) case ARG0(DEPAREN(ITEM)): return msg.ARG0(DEPAREN(ITEM)).wrap(msg.ARG0(DEPAREN(ITEM)).parse(payload));
#define _DELETE(ITEM) case msg::Kind::ARG0(DEPAREN(ITEM)): delete (msg::ARG0(DEPAREN(ITEM))*) msg->data; break;

#define QB_ENGINE(DRIVERS, MSGS...) \
struct QB: public quimblos::Engine { \
    DRIVERS \
    struct msg { \
        enum Kind { \
            MAP_LIST(_ENUM, MSGS) \
        }; \
        MAP(_STRUCT, MSGS) \
    } msg; \
    const quimblos::msg_wrap_t* parse(uint8_t kind, const std::string& payload) { \
        switch (kind) { \
            MAP(_PARSE, MSGS) \
        } \
    } \
    void _delete(const quimblos::msg_wrap_t* msg) { \
        switch (msg->kind) { \
            MAP(_DELETE, MSGS) \
        } \
    } \
}; \
static QB qb;

#define _ARG1D(X, ...) _ARG0(__VA_ARGS__)
#define ARG1D(X) _ARG1D(X)

#define _ARG2D(X, ...) _ARG1(__VA_ARGS__)
#define ARG2D(X) _ARG2D(X)

#define __QB_DRIVER_DECL(X) ARG0(DEPAREN(X)) ARG1D(DEPAREN(X));
#define __QB_DRIVER_INIT(X) ARG1D(DEPAREN(X)) ARG2D(DEPAREN(X))
#define __QB_DRIVER_NAME(X) &ARG1D(DEPAREN(X))

#define QB_DRIVERS(DRIVERS...) \
    MAP(__QB_DRIVER_DECL, DRIVERS) \
    QB(): \
        quimblos::Engine({ \
            MAP_LIST(__QB_DRIVER_NAME, DRIVERS) \
        }), \
        MAP_LIST(__QB_DRIVER_INIT, DRIVERS) \
    {}
    

#define QB_MSG(NAME, STRUCT, SCHEMA...) \
    (NAME, struct NAME { \
        DEPAREN(STRUCT) \
        const quimblos::msg_wrap_t* wrap(const msg::NAME* msg) { return new quimblos::msg_wrap_t({ .kind = (uint8_t) NAME, .data = msg }); } \
        const msg::NAME& unwrap(const quimblos::msg_wrap_t& wrap) { return *(const msg::NAME*) wrap.data; } \
        const quimblos::serial::Schema schema(msg::NAME* msg) { return { SCHEMA }; } \
        const std::string& serial(msg::NAME* msg) { \
            return quimblos::serial::dump(schema(msg), msg::Kind::NAME); \
        } \
        const msg::NAME* parse(const std::string& payload) { \
            auto msg = new msg::NAME(); \
            quimblos::serial::parse(schema(msg), payload); \
            return msg; \
        } \
    } NAME;);
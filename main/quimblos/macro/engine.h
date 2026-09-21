#pragma once

#include "deparen.h"
#include "map.h"
#include "arg.h"

#define __QB_MSG_NAME(X) ARG0(DEPAREN(X))
#define __QB_MSG_STRUCT(X) ARG1OF2(DEPAREN(X))
#define __QB_MSG_PARSE_CASE(X) case msg_t::__QB_MSG_NAME(X): return msg::__QB_MSG_NAME(X)::parse(payload)->wrap();

#define __QB_DRIVER_CLASS(X) ARG0(DEPAREN(X))
#define __QB_DRIVER_NAME(X) ARG1OF3(DEPAREN(X))
#define __QB_DRIVER_DECL(X) static auto __QB_DRIVER_NAME(X) = ARG0(DEPAREN(X)) ARG2OF3(DEPAREN(X));
#define __QB_DRIVER_BIND(X) __QB_DRIVER_CLASS(X)::engine = this;

#define __QB_BOOT_DRIVER(X) driver::__QB_DRIVER_NAME(X).load();

#define QB_ENGINE(NS, DRIVERS, MSGS...) \
namespace NS { \
    enum msg_t: uint8_t { \
        MAP_LIST(__QB_MSG_NAME, MSGS) \
    }; \
    namespace msg { \
        MAP(__QB_MSG_STRUCT, MSGS) \
    } \
    namespace driver { \
        MAP(__QB_DRIVER_DECL, DRIVERS) \
    } \
    class Engine : public qb::Engine { \
        public: \
            Engine() { \
                MAP(__QB_DRIVER_BIND, DRIVERS) \
            } \
            const qb::msg_wrap_t* parse(uint8_t kind, const std::string& payload) const { \
                switch (kind) { \
                    MAP(__QB_MSG_PARSE_CASE, MSGS) \
                } \
                return nullptr; \
            } \
            void boot() const { \
                MAP(__QB_BOOT_DRIVER, DRIVERS) \
            } \
    };\
}

#define QB_DRIVERS(DRIVERS...) DRIVERS

#define QB_MSG(NAME, PROPS, SCHEMA...) \
    (NAME, struct NAME { \
        DEPAREN(PROPS) \
        const qb::msg_wrap_t* wrap() const { return new qb::msg_wrap_t({ .kind = msg_t::NAME, .data = this }); } \
        static const NAME& unwrap(const qb::msg_wrap_t& wrap) { return *(const NAME*) wrap.data; } \
        const qb::serial::Schema schema() const { return { SCHEMA }; } \
        static const NAME* parse(const std::string& payload) { \
            auto msg = new NAME(); \
            qb::serial::parse(msg->schema(), payload); \
            return msg; \
        } \
        const std::string dump() const { \
            return qb::serial::dump(schema(), msg_t::NAME); \
        } \
    };)

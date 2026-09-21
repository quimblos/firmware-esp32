#pragma once

#include "deparen.h"
#include "map.h"
#include "arg.h"

#define __QB_MSG_NAME(X) ARG0(DEPAREN(X))
#define __QB_MSG_STRUCT(X) DEPAREN(ARG1OF3(DEPAREN(X)))
#define __QB_MSG_TO_JSON(X) ARG2OF3(DEPAREN(X))
#define __QB_MSG_PARSE_CASE(X) case msg_t::__QB_MSG_NAME(X): return msg::__QB_MSG_NAME(X)::parse(payload)->wrap();
#define __QB_MSG_TO_JSON_CASE(X) case msg_t::__QB_MSG_NAME(X): qb::to_json(os, *(msg::__QB_MSG_NAME(X)*) wrap->data); break;

#define __QB_DRIVER_CLASS(X) ARG0(DEPAREN(X))
#define __QB_DRIVER_NAME(X) ARG1OF3(DEPAREN(X))
#define __QB_DRIVER_DECL(X) static auto __QB_DRIVER_NAME(X) = ARG0(DEPAREN(X)) ARG2OF3(DEPAREN(X));
#define __QB_DRIVER_BIND(X) __QB_DRIVER_CLASS(X)::engine = this;

#define __QB_BOOT_DRIVER(X) driver::__QB_DRIVER_NAME(X).load();

// Data Types

#define __QB_STRUCT_FIELD(X) ARG1OF2(DEPAREN(X)) ARG0(DEPAREN(X));
#define __QB_STRUCT_FIELD_TO_JSON(X) << '"' << TARG0(DEPAREN(X)) << "\":"; qb::to_json(os, obj.ARG0(DEPAREN(X))); os <<

#define __QB_OBJ_STRUCT(X) DEPAREN(ARG0(DEPAREN(X)))
#define __QB_OBJ_TO_JSON(X) ARG1OF2(DEPAREN(X))

#define QB_DATA(NS, ...) \
    namespace NS { \
        namespace data { \
            MAP(__QB_OBJ_STRUCT, __VA_ARGS__) \
        } \
    } \
    using namespace NS; \
    namespace qb { \
        MAP(__QB_OBJ_TO_JSON, __VA_ARGS__) \
    }

#define QB_OBJ(NAME, FIELDS, ASSERTS...) \
    ((struct NAME { \
        MAP(__QB_STRUCT_FIELD, DEPAREN(FIELDS)) \
        esp_err_t validate() const { \
            ASSERTS \
            return ESP_OK; \
        } \
    };), \
    template <> \
    inline std::ostream& to_json<data::NAME>(std::ostream& os, const data::NAME& obj) { \
        os << '{' MAP_CLIST(__QB_STRUCT_FIELD_TO_JSON, DEPAREN(FIELDS)) '}'; \
        return os; \
    })

#define QB_VEC(NAME) \
    ((),template <> \
    inline std::ostream& to_json<std::vector<NAME>>(std::ostream& os, const std::vector<NAME>& obj) { \
        os << '['; \
        for (const auto& it: obj) qb::to_json(os, it) << ',';\
        os << ']'; \
        return os; \
    })

// Engine

#define QB_ENGINE(NS, DRIVERS, MSGS...) \
namespace NS { \
    enum msg_t: uint8_t { \
        MAP_LIST(__QB_MSG_NAME, MSGS) \
    }; \
    namespace msg { \
        MAP(__QB_MSG_STRUCT, MSGS) \
    } \
} \
using namespace NS; \
namespace qb { \
    MAP(__QB_MSG_TO_JSON, MSGS) \
} \
namespace NS { \
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
            const std::string unwrap_json(const qb::msg_wrap_t* wrap) const { \
                std::ostringstream os; \
                switch (wrap->kind) { \
                    MAP(__QB_MSG_TO_JSON_CASE, MSGS) \
                } \
                return os.str(); \
            } \
            void boot() const { \
                MAP(__QB_BOOT_DRIVER, DRIVERS) \
            } \
    }; \
}

#define QB_DRIVERS(DRIVERS...) DRIVERS

#define QB_MSG(NAME, FIELDS, SCHEMA...) \
    (NAME, (struct NAME { \
        MAP(__QB_STRUCT_FIELD, DEPAREN(FIELDS)) \
        inline const qb::msg_wrap_t* wrap() const { return new qb::msg_wrap_t({ .kind = msg_t::NAME, .data = this }); } \
        inline static const NAME& unwrap(const qb::msg_wrap_t& wrap) { return *(const NAME*) wrap.data; } \
        inline const qb::serial::Schema schema() const { return { SCHEMA }; } \
        inline static const NAME* parse(const std::string& payload) { \
            auto msg = new NAME(); \
            qb::serial::parse(msg->schema(), payload); \
            return msg; \
        } \
        inline const std::string dump() const { \
            return qb::serial::dump(schema(), msg_t::NAME); \
        } \
    };), \
    template <> \
    inline std::ostream& to_json<msg::NAME>(std::ostream& os, const msg::NAME& obj) { \
        os << '{' MAP_CLIST(__QB_STRUCT_FIELD_TO_JSON, DEPAREN(FIELDS)) '}'; \
        return os; \
    })

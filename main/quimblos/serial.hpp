#pragma once

#include <cstdlib>
#include <sstream>
#include <string>
#include <vector>
#include <iomanip>
#include <cstdint>

namespace quimblos {

    namespace serial {

        struct Prop {
            enum Type {
                UINT8,
                INT8,
                UINT16,
                INT16,
                UINT32,
                INT32,
            } type;
            void* data;
            bool is_array = false;
        };

        typedef std::vector<Prop> Schema;

        uint8_t chhex(const char& ch) {
            switch (ch) {
                case '0': return 0;
                case '1': return 1;
                case '2': return 2;
                case '3': return 3;
                case '4': return 4;
                case '5': return 5;
                case '6': return 6;
                case '7': return 7;
                case '8': return 8;
                case '9': return 9;
                case 'A': return 10;
                case 'a': return 10;
                case 'B': return 11;
                case 'b': return 11;
                case 'C': return 12;
                case 'c': return 12;
                case 'D': return 13;
                case 'd': return 13;
                case 'E': return 14;
                case 'e': return 14;
                case 'F': return 15;
                case 'f': return 15;
            }
            return 0;
        }


        std::string dump(const serial::Schema schema, uint8_t kind) {
            std::ostringstream ss;
            ss << std::hex << std::setfill('0') << std::setw(2) << kind;
            
            for (const auto& it: schema) {
                ss << std::hex << std::setfill('0') << std::setw(2) << it.type;
                switch (it.type) {
                    case serial::Prop::UINT8:
                    case serial::Prop::INT8:
                        ss << std::hex << std::setfill('0') << std::setw(2) << *(int8_t*) it.data; break;
                    case serial::Prop::UINT16:
                    case serial::Prop::INT16:
                        ss << std::hex << std::setfill('0') << std::setw(4) << *(int8_t*) it.data; break;
                    case serial::Prop::UINT32:
                    case serial::Prop::INT32:
                        ss << std::hex << std::setfill('0') << std::setw(8) << *(int8_t*) it.data; break;
                }
            }

            return ss.str();
        }

        void parse(const serial::Schema schema, const std::string& payload) {
            const char* p = payload.c_str();
            uint8_t d = 0;
            for (size_t i = 2; i < payload.size();) {

                serial::Prop::Type type = (serial::Prop::Type) (serial::chhex(p[i])*16 + serial::chhex(p[i+1]));
                i += 2;

                switch (type) {
                    case serial::Prop::UINT8:
                        *(uint8_t*) schema[d].data = (serial::chhex(p[i])*16 + serial::chhex(p[i+1]));
                        i += 2;
                        break;
                    case serial::Prop::INT8:
                        *(int8_t*) schema[d].data = (serial::chhex(p[i])*16 + serial::chhex(p[i+1]));
                        i += 2;
                        break;
                    case serial::Prop::UINT16:
                        *(uint16_t*) schema[d].data = (serial::chhex(p[i])*4096 + serial::chhex(p[i+1])*256 + serial::chhex(p[i+2])*16 + serial::chhex(p[i+3]));
                        i += 4;
                        break;
                    case serial::Prop::INT16:
                        *(int16_t*) schema[d].data = (serial::chhex(p[i])*4096 + serial::chhex(p[i+1])*256 + serial::chhex(p[i+2])*16 + serial::chhex(p[i+3]));
                        i += 4;
                        break;
                    case serial::Prop::UINT32:
                        *(uint32_t*) schema[d].data = (serial::chhex(p[i])*268435456 + serial::chhex(p[i+1])*16777216 + serial::chhex(p[i+2])*1048576 + serial::chhex(p[i+3])*65536 + serial::chhex(p[i+4])*4096 + serial::chhex(p[i+5])*256 + serial::chhex(p[i+6])*16 + serial::chhex(p[i+7]));
                        i += 8;
                        break;
                    case serial::Prop::INT32:
                        *(int32_t*) schema[d].data = (serial::chhex(p[i])*268435456 + serial::chhex(p[i+1])*16777216 + serial::chhex(p[i+2])*1048576 + serial::chhex(p[i+3])*65536 + serial::chhex(p[i+4])*4096 + serial::chhex(p[i+5])*256 + serial::chhex(p[i+6])*16 + serial::chhex(p[i+7]));
                        i += 8;
                        break;
                }
                d++;
            }
        }
    }

    
}
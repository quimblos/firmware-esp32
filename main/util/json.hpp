#pragma once

#include <unordered_map>
#include <cctype>      // std::isspace
#include <cstring>     // std::strchr
#include <string>     // std::strchr

struct JSON {
    static const JSON empty;
    enum Kind {
        EMPTY,
        VAL,
        OBJ,
        ARR
    } kind;
    std::string value;
    std::unordered_map<std::string, JSON> children;

    inline static JSON parse(const std::string& payload) {
        size_t i = 0;
        return parse_value(payload, i);
    }

    const JSON& get(const std::string& key) const {
        if (children.contains(key)) return children.at(key);
        return JSON::empty;
    }

    private:
        inline static void ws(const std::string& s, size_t& i) {
            while (i < s.size() && std::isspace((unsigned char) s[i])) ++i;
        }

        // s[i] == '"': returns the unescaped contents, advances past the closing quote
        inline static std::string str(const std::string& s, size_t& i) {
            std::string out;
            for (++i; i < s.size() && s[i] != '"'; ++i) {
                if (s[i] == '\\' && i + 1 < s.size()) {
                    switch (s[++i]) {
                        case 'n':  out += '\n'; break;
                        case 't':  out += '\t'; break;
                        case 'r':  out += '\r'; break;
                        case 'b':  out += '\b'; break;
                        case 'f':  out += '\f'; break;
                        default:   out += s[i];
                    }
                } else {
                    out += s[i];
                }
            }
            ++i;  // closing quote
            return out;
        }

        // Parses one value at i and leaves i just past it.
        inline static JSON parse_value(const std::string& s, size_t& i) {
            ws(s, i);
            JSON node{ .kind = VAL, .value = "", .children = {} };
            if (i >= s.size()) return node;

            if (s[i] == '{' || s[i] == '[') {
                const bool obj   = s[i] == '{';
                const char close = obj ? '}' : ']';
                node.kind = obj ? OBJ : ARR;

                ws(s, ++i);
                for (size_t n = 0; i < s.size() && s[i] != close; ++n) {
                    std::string key;
                    if (obj) {
                        ws(s, i);
                        key = str(s, i);          // member name
                        ws(s, i);
                        if (i < s.size() && s[i] == ':') ++i;
                    } else {
                        key = std::to_string(n);  // array element index
                    }

                    node.children[key] = parse_value(s, i);

                    ws(s, i);
                    if (i < s.size() && s[i] == ',') ++i;
                    ws(s, i);
                }
                if (i < s.size()) ++i;  // closing bracket
            } else if (s[i] == '"') {
                node.value = str(s, i);
            } else {
                const size_t start = i;
                while (i < s.size() && !std::strchr(",}] \t\r\n", s[i])) ++i;
                node.value = s.substr(start, i - start);
            }
            return node;
        }
};
#ifndef __CJSON_JSON_UTILS_H__
#define __CJSON_JSON_UTILS_H__
#include "ccore/c_target.h"
#ifdef USE_PRAGMA_ONCE
#pragma once
#endif

#include "ccore/c_debug.h"

namespace ncore
{
    namespace njson
    {
        enum JsonNumberType
        {
            kJsonNumber_unknown = 0x0,
            kJsonNumber_bool    = 0x1,
            kJsonNumber_s64     = 0x2,
            kJsonNumber_u64     = 0x4,
            kJsonNumber_f64     = 0x8,
        };

        struct JsonNumber
        {
            JsonNumber() : m_Type(kJsonNumber_unknown), m_U64(0) {}
            JsonNumber(JsonNumberType type) : m_Type(type), m_U64(0) {}
            JsonNumber(JsonNumberType type, s64 s) : m_Type(type), m_U64(s) {}

            u32 m_Type;
            union
            {
                s64 m_S64;
                u64 m_U64;
                f64 m_F64;
            };
        };

        bool ParseBoolean(char const* str, char const* end);
        bool ParseHexNumber(char const*& str, char const* end, JsonNumber& out_number);
        bool ParseNumber(char const*& str, char const* end, JsonNumber& out_number);
        u64  ParseMacAddress(char const* str, char const* end);

        bool        JsonNumberIsValid(JsonNumber const& number);
        s64         JsonNumberAsInt64(JsonNumber const& number);
        u64         JsonNumberAsUInt64(JsonNumber const& number);
        f64         JsonNumberAsFloat64(JsonNumber const& number);

        // UTF-8; read a character and return the unicode codepoint (UTF-32)
        struct uchar8_t
        {
            u32 c;
            s32 l;
        };
        bool        WriteChar(u32 ch, char*& cursor, char const* end);
        s32         SizeOfChar(u32 ch);
        uchar8_t    PeekUtf8Char(const char* str, const char* end);
        inline char PeekAsciiChar(const char* str, const char* end)
        {
            if (str < end)
                return *str;
            return '\0';
        }

        void  EnumToString(u64 e, const char** enum_strs, const u64* enum_values, i32 enum_count, char*& str, const char* end);
        void  EnumFromString(const char*& str, const char** enum_strs, const u64* enum_values, i32 enum_count, u64& out_e);

        void  FlagsToString(u64 e, const char** enum_strs, const u64* enum_values, i32 enum_count, char*& str, const char* end);
        void  FlagsFromString(const char*& str, const char** enum_strs, const u64* enum_values, i32 enum_count, u64& out_e);

    } // namespace json
} // namespace ncore

#endif // __CJSON_JSON_UTILS_H__

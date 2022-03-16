#ifndef __XBASE_JSON_UTILS_H__
#define __XBASE_JSON_UTILS_H__
#include "xbase/x_target.h"
#ifdef USE_PRAGMA_ONCE
#pragma once
#endif

#include "xbase/x_debug.h"

namespace xcore
{
    namespace json
    {
        enum JsonNumberType
        {
            kJsonNumber_unknown = 0x0,
            kJsonNumber_s64     = 0x1,
            kJsonNumber_u64     = 0x2,
            kJsonNumber_f64     = 0x4,
        };

        struct JsonNumber
        {
            u32 m_Type;
            union
            {
                s64 m_S64;
                u64 m_U64;
                f64 m_F64;
            };
        };

        char const* ParseNumber(char const* str, char const* end, JsonNumber& out_number);
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

        void  EnumToString(u64 e, const char** enum_strs, char*& str, const char* end);
        void  EnumFromString(const char* str, const char** enum_strs, u64& out_e);

    } // namespace json
} // namespace xcore

#endif // __XBASE_JSON_H__
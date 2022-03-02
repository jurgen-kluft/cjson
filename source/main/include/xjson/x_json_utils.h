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
            kJsonNumber_unknown = 0,
            kJsonNumber_s64     = 0x0001,
            kJsonNumber_u64     = 0x0002,
            kJsonNumber_s32     = 0x0004,
            kJsonNumber_u32     = 0x0008,
            kJsonNumber_s16     = 0x0010,
            kJsonNumber_u16     = 0x0020,
            kJsonNumber_s8      = 0x0040,
            kJsonNumber_u8      = 0x0080,
            kJsonNumber_f32     = 0x0100,
            kJsonNumber_f64     = 0x0200,
        };

        struct JsonNumber
        {
            u32 m_Type;
            union
            {
                s64    m_S64;
                s32    m_S32;
                s16    m_S16;
                s8     m_S8;
                u64    m_U64;
                u32    m_U32;
                u16    m_U16;
                u8     m_U8;
                float  m_F32;
                double m_F64;
            };
        };

        char const* ParseNumber(char const* str, char const* end, JsonNumber& out_number);
		bool        JsonNumberIsValid(JsonNumber const& number);
        s64         JsonNumberAsInt64(JsonNumber const& number);
        u64         JsonNumberAsUInt64(JsonNumber const& number);
        s32         JsonNumberAsInt32(JsonNumber const& number);
        u32         JsonNumberAsUInt32(JsonNumber const& number);
        s16         JsonNumberAsInt16(JsonNumber const& number);
        u16         JsonNumberAsUInt16(JsonNumber const& number);
        s8          JsonNumberAsInt8(JsonNumber const& number);
        u8          JsonNumberAsUInt8(JsonNumber const& number);
        f32         JsonNumberAsFloat32(JsonNumber const& number);
        f64         JsonNumberAsFloat64(JsonNumber const& number);

        // UTF-8; read a character and return the unicode codepoint (UTF-32)
        struct uchar8_t
        {
            u32 c;
            s32 l;
        };
        bool     WriteChar(u32 ch, char*& cursor, char const* end);
        s32      SizeOfChar(u32 ch);
        uchar8_t PeekChar(const char* str, const char* end);

    } // namespace json
} // namespace xcore

#endif // __XBASE_JSON_H__
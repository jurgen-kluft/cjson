#include "xbase/x_allocator.h"
#include "xbase/x_context.h"
#include "xbase/x_printf.h"
#include "xbase/x_runes.h"
#include "xjson/x_json_utils.h"

namespace xcore
{
    namespace json
    {
        uchar8_t PeekChar(const char* str, const char* end)
        {
            if (str >= end)
                return {0, 0};

            uchar8_t c;
            c.c = (unsigned char)*str;
            c.l = 0;
            if (c.c != 0)
            {
                if (c.c < 0x80)
                {
                    c.l = 1;
                }
                else if ((c.c >> 5) == 0x6)
                {
                    c.c = ((c.c << 6) & 0x7ff) + ((str[1]) & 0x3f);
                    c.l = 2;
                }
                else if ((c.c >> 4) == 0xe)
                {
                    c.c = ((c.c << 12) & 0xffff) + (((str[1]) << 6) & 0xfff);
                    c.c += (str[2]) & 0x3f;
                    c.l = 3;
                }
                else if ((c.c >> 3) == 0x1e)
                {
                    c.c = ((c.c << 18) & 0x1fffff) + (((str[1]) << 12) & 0x3ffff);
                    c.c += ((str[2]) << 6) & 0xfff;
                    c.c += (str[3]) & 0x3f;
                    c.l = 4;
                }
                else
                {
                    c.c = 0xFFFE; // illegal character
                    c.l = -1;
                }
            }
            return c;
        }

        s32 SizeOfChar(u32 ch)
        {
            if (ch < 0x80)
            {
                return 1;
            }
            else if (ch < 0x800)
            {
                return 2;
            }
            else if (ch < 0x10000)
            {
                return 3;
            }
            else if (ch < 0x200000)
            {
                return 4;
            }
            return -1;
        }

        bool WriteChar(u32 ch, char*& cursor, char const* end)
        {
            if (ch < 0x80)
            { // one octet
                if (cursor < end)
                {
                    *(cursor++) = static_cast<uchar8>(ch);
                    return true;
                }
            }
            else if (ch < 0x800)
            { // two octets
                if ((cursor + 1) < end)
                {
                    *(cursor++) = static_cast<uchar8>((ch >> 6) | 0xc0);
                    *(cursor++) = static_cast<uchar8>((ch & 0x3f) | 0x80);
                    return true;
                }
            }
            else if (ch < 0x10000)
            { // three octets
                if ((cursor + 2) < end)
                {
                    *(cursor++) = static_cast<uchar8>((ch >> 12) | 0xe0);
                    *(cursor++) = static_cast<uchar8>(((ch >> 6) & 0x3f) | 0x80);
                    *(cursor++) = static_cast<uchar8>((ch & 0x3f) | 0x80);
                    return true;
                }
            }
            else
            { // four octets
                if ((cursor + 3) < end)
                {
                    *(cursor++) = static_cast<uchar8>((ch >> 18) | 0xf0);
                    *(cursor++) = static_cast<uchar8>(((ch >> 12) & 0x3f) | 0x80);
                    *(cursor++) = static_cast<uchar8>(((ch >> 6) & 0x3f) | 0x80);
                    *(cursor++) = static_cast<uchar8>((ch & 0x3f) | 0x80);
                    return true;
                }
            }
            return false;
        }

        char const* ParseNumber(char const* str, char const* end, JsonNumber& out_number)
        {
            out_number.m_Type = kJsonNumber_unknown;

            u64 integer = 0;
            f64 number  = 0.0;

            // If the number is negative
            s8       sign = 1;
            uchar8_t c    = PeekChar(str, end);
            if (c.c == '-')
            {
                sign = -1;
                str += c.l;
            }

            // Parse the integer part.
            while (str < end)
            {
                c = PeekChar(str, end);
                if (c.c < '0' || c.c > '9')
                {
                    break;
                }
                integer = integer * 10 + (c.c - '0');
                str += c.l;
            }

            number = (f64)integer;
            number *= sign;

            // Parse the decimal part.
            c = PeekChar(str, end);
            if (str < end && c.c == '.')
            {
                str += c.l;
                f64 decimal = 0.0;
                f64 div     = 1.0;
                while (str < end)
                {
                    c = PeekChar(str, end);
                    if (c.c < '0' || c.c > '9')
                    {
                        break;
                    }
                    decimal = decimal * 10.0 + (c.c - '0');
                    div *= 10.0;
                    str += c.l;
                }
                number = decimal / div;

                out_number.m_Type |= kJsonNumber_f64;
                out_number.m_F64 = number;
            }

            // Parse the exponent part.
            c = PeekChar(str, end);
            if (str < end && (c.c == 'e' || c.c == 'E'))
            {
                str += c.l;

                s32 esign    = 1;
                s32 exponent = 0;
                c            = PeekChar(str, end);
                if (str < end && (c.c == '+' || c.c == '-'))
                {
                    if (c.c == '-')
                    {
                        esign = -1;
                    }
                    str += c.l;
                }
                while (str < end)
                {
                    uchar8_t c = PeekChar(str, end);
                    if (c.c < '0' || c.c > '9')
                    {
                        break;
                    }
                    exponent = exponent * 10 + (c.c - '0');
                    str += c.l;
                }
                if (exponent > 308)
                {
                    exponent = 308;
                }

                // Adjust result on exponent esign
                if (esign > 0)
                {
                    for (s32 i = 0; i < exponent; i++)
                        number *= 10.0;
                }
                else
                {
                    for (s32 i = 0; i < exponent; i++)
                        number /= 10.0;
                }

                out_number.m_Type |= kJsonNumber_f64;
                out_number.m_F64 = number;
            }

            if (out_number.m_Type == kJsonNumber_unknown)
            {
                if (sign == 1)
                {
                    out_number.m_Type = kJsonNumber_u64;
                    if (integer <= 9223372036854775807ul)
                        out_number.m_Type |= kJsonNumber_s64;
                    out_number.m_U64 = (u64)integer;
				}
				else
				{
                    out_number.m_Type = kJsonNumber_s64;
                    out_number.m_S64 = (s64)(sign * integer);
				}
            }

            return str;
        }

		bool        JsonNumberIsValid(JsonNumber const& number)
		{
			return number.m_Type != kJsonNumber_unknown;
		}

        s64         JsonNumberAsInt64(JsonNumber const& number)
        {
            else if (number.m_Type & kJsonNumber_s64)
            {
                return number.m_S64;
            }
            else if (number.m_Type & kJsonNumber_u8)
            {
                return number.m_U8;
            }
            else if (number.m_Type & kJsonNumber_u16)
            {
                return number.m_U16;
            }
            else if (number.m_Type & kJsonNumber_u32)
            {
                return number.m_U32;
            }
            else if (number.m_Type & kJsonNumber_u64)
            {
                return number.m_U64;
            }
            else if (number.m_Type & kJsonNumber_f32)
            {
                return (s64)number.m_F32;
            }
            else if (number.m_Type & kJsonNumber_f64)
            {
                return (s64)number.m_F64;
            }
            else
            {
                return 0;
            }
        }

        u64         JsonNumberAsUInt64(JsonNumber const& number)
        {
            if (number.m_Type & kJsonNumber_s8)
            {
                return (u64)(s8)number.m_S8;
            }
            else if (number.m_Type & kJsonNumber_s16)
            {
                return (u64)(s16)number.m_S16;
            }
            else if (number.m_Type & kJsonNumber_s32)
            {
                return (u64)(s32)number.m_S32;
            }
            else if (number.m_Type & kJsonNumber_s64)
            {
                return (u64)(s64)number.m_S64;
            }
            else if (number.m_Type & kJsonNumber_u8)
            {
                return number.m_U8;
            }
            else if (number.m_Type & kJsonNumber_u16)
            {
                return number.m_U16;
            }
            else if (number.m_Type & kJsonNumber_u32)
            {
                return number.m_U32;
            }
            else if (number.m_Type & kJsonNumber_u64)
            {
                return number.m_U64;
            }
            else if (number.m_Type & kJsonNumber_f32)
            {
                return (u64)number.m_F32;
            }
            else if (number.m_Type & kJsonNumber_f64)
            {
                return (u64)number.m_F64;
            }
            else
            {
                return 0;
            }
        }

        s32         JsonNumberAsInt32(JsonNumber const& number)
        {
            return (s32)JsonNumberAsInt64(number);
        }
        u32         JsonNumberAsUInt32(JsonNumber const& number)
        {
            return (u32)JsonNumberAsUInt64(number);
        }
        s16         JsonNumberAsInt16(JsonNumber const& number)
        {
            return (s16)JsonNumberAsInt64(number);
        }
        u16         JsonNumberAsUInt16(JsonNumber const& number)
        {
            return (u16)JsonNumberAsUInt64(number);
        }
        s8          JsonNumberAsInt8(JsonNumber const& number)
        {
            return (s8)JsonNumberAsInt64(number);
        }
        u8          JsonNumberAsUInt8(JsonNumber const& number)
        {
            return (u8)JsonNumberAsUInt64(number);
        }
        f64         JsonNumberAsFloat64(JsonNumber const& number)
        {
            if (number.m_Type & kJsonNumber_s8)
            {
                return (f64)number.m_S8;
            }
            else if (number.m_Type & kJsonNumber_s16)
            {
                return (f64)number.m_S16;
            }
            else if (number.m_Type & kJsonNumber_s32)
            {
                return (f64)number.m_S32;
            }
            else if (number.m_Type & kJsonNumber_s64)
            {
                return (f64)number.m_S64;
            }
            else if (number.m_Type & kJsonNumber_u8)
            {
                return (f64)number.m_U8;
            }
            else if (number.m_Type & kJsonNumber_u16)
            {
                return (f64)number.m_U16;
            }
            else if (number.m_Type & kJsonNumber_u32)
            {
                return (f64)number.m_U32;
            }
            else if (number.m_Type & kJsonNumber_u64)
            {
                return (f64)number.m_U64;
            }
            else if (number.m_Type & kJsonNumber_f32)
            {
                return (f64)number.m_F32;
            }
            else if (number.m_Type & kJsonNumber_f64)
            {
                return number.m_F64;
            }
            else
            {
                return 0.0;
            }
        }
        f32         JsonNumberAsFloat32(JsonNumber const& number)
        {
            return (f32)JsonNumberAsFloat64(number);
        }

    } // namespace json
} // namespace xcore
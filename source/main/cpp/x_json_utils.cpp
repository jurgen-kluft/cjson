#include "xbase/x_allocator.h"
#include "xbase/x_context.h"
#include "xbase/x_printf.h"
#include "xbase/x_runes.h"
#include "xjson/x_json_utils.h"

namespace xcore
{
    namespace json
    {
        uchar8_t PeekUtf8Char(const char* str, const char* end)
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
            s64  sign = 1;
            char c    = PeekAsciiChar(str, end);
            if (c == '-')
            {
                sign = -1;
                str++;
            }

            // Parse the integer part.
            while (str < end)
            {
                c = PeekAsciiChar(str, end);
                if (c < '0' || c > '9')
                {
                    break;
                }
                integer = integer * 10 + (c - '0');
                str++;
            }

            number = (f64)integer;
            number *= sign;

            // Parse the decimal part.
            c = PeekAsciiChar(str, end);
            if (str < end && c == '.')
            {
                str++;
                f64 decimal = 0.0;
                f64 div     = 1.0;
                while (str < end)
                {
                    c = PeekAsciiChar(str, end);
                    if (c < '0' || c > '9')
                    {
                        break;
                    }
                    decimal = decimal * 10.0 + (c - '0');
                    div *= 10.0;
                    str++;
                }
                number += decimal / div;

                out_number.m_Type = kJsonNumber_f64;
                out_number.m_F64 = number;
            }

            // Parse the exponent part.
            c = PeekAsciiChar(str, end);
            if (str < end && (c == 'e' || c == 'E'))
            {
                str++;

                s32 esign    = 1;
                s32 exponent = 0;
                c            = PeekAsciiChar(str, end);
                if (str < end && (c == '+' || c == '-'))
                {
                    if (c == '-')
                    {
                        esign = -1;
                    }
                    str++;
                }
                while (str < end)
                {
                    c = PeekAsciiChar(str, end);
                    if (c < '0' || c > '9')
                    {
                        break;
                    }
                    exponent = exponent * 10 + (c - '0');
                    str++;
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

                out_number.m_Type = kJsonNumber_f64;
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
                    out_number.m_S64  = (s64)sign * (s64)integer;
                }
            }

            return str;
        }

        bool JsonNumberIsValid(JsonNumber const& number) { return number.m_Type != kJsonNumber_unknown; }

        s64 JsonNumberAsInt64(JsonNumber const& number)
        {
            if (number.m_Type & kJsonNumber_s64)
            {
                return number.m_S64;
            }
            else if (number.m_Type & kJsonNumber_u64)
            {
                return (s64)number.m_U64;
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

        u64 JsonNumberAsUInt64(JsonNumber const& number)
        {
            if (number.m_Type & kJsonNumber_s64)
            {
                return (u64)(s64)number.m_S64;
            }
            else if (number.m_Type & kJsonNumber_u64)
            {
                return number.m_U64;
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

        f64 JsonNumberAsFloat64(JsonNumber const& number)
        {
            if (number.m_Type & kJsonNumber_s64)
            {
                return (f64)number.m_S64;
            }
            else if (number.m_Type & kJsonNumber_u64)
            {
                return (f64)number.m_U64;
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


        static void json_write_str(char*& dst, char const* end, char const* str)
        {
            while (dst < end && *str)
            {
                *dst++ = *str++;
            }
            *dst = '\0';
        }

        void EnumToString(u64 e, const char** enum_strs, char*& str, const char* end)
        {
            char* begin = str;
            char* dst   = begin;

            s16 i = 0;
            while (e != 0 && (enum_strs[i] != nullptr))
            {
                u16 bit = (u16)(e & 1);
                if (bit != 0)
                {
                    json_write_str(dst, end, enum_strs[i]);
                }

                i++;
                e = e >> 1;

                if (e != 0)
                {
                    json_write_str(dst, end, "|");
                }
            }

            *dst = '\0';
            str = dst;
        }

        static bool json_enum_equal(const char*& enum_str, const char* str)
        {
            const char* estr = enum_str;
            while (true)
            {
                char ec = *estr;
                char c = *str;
                if ((ec == '\0' || ec == '|') && c == '\0')
                    break;
                    
                if (c == '\0')
                    return false;

                if (ec != c)
                {
                    ec = to_lower(ec);
                    c = to_lower(c);
                    if (ec != c)
                        return false;
                }
                estr++;
                str++;
            }
            enum_str = estr;
            return true;
        }

        void EnumFromString(const char* str, const char** enum_strs, u64& out_e)
        {
            const char** estr = enum_strs;
            while (true)
            {
                while (*str != '\0' && is_whitespace(*str))
                {
                    str++;
                }
                if (*str == '\0')
                    break;

                if (*str == '|')
                {
                    str++;
                    continue;
                }

                s32 e = 0;
                while (enum_strs[e] != nullptr)
                {
                    if (json_enum_equal(str, enum_strs[e]))
                    {
                        out_e |= (u64)1 << e;
                        break;
                    }
                    e++;
                }
            }
        }


    } // namespace json
} // namespace xcore
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

        char const* ParseNumber(char const* str, char const* end, f64* out_number)
        {
            f64 number = 0.0;

            // If the number is negative
            uchar8_t c = PeekChar(str, end);
            if (c.c == '-')
            {
                number = -1.0;
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
                number = number * 10.0 + (c.c - '0');
                str += c.l;
            }

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
                number += decimal / div;
            }

            // Parse the exponent part.
            c = PeekChar(str, end);
            if (str < end && (c.c == 'e' || c.c == 'E'))
            {
                str += c.l;

                s32 sign     = 1;
                s32 exponent = 0;
                c            = PeekChar(str, end);
                if (str < end && (c.c == '+' || c.c == '-'))
                {
                    if (c.c == '-')
                    {
                        sign = -1;
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

                // Adjust result on exponent sign
                if (sign > 0)
                {
                    for (s32 i = 0; i < exponent; i++)
                        number *= 10.0;
                }
                else
                {
                    for (s32 i = 0; i < exponent; i++)
                        number /= 10.0;
                }
            }

            *out_number = number;
            return str;
        }

    } // namespace json
} // namespace xcore
#include "xbase/x_allocator.h"
#include "xbase/x_context.h"
#include "xbase/x_printf.h"
#include "xbase/x_runes.h"
#include "xjson/x_json.h"
#include "xjson/x_json_utils.h"
#include "xjson/x_json_allocator.h"
#include "xjson/x_json_lexer.h"

namespace xcore
{
    namespace json
    {
        void JsonLexerStateInit(JsonLexerState* self, char const* buffer, char const* end, JsonAllocator* alloc, JsonAllocator* scratch)
        {
            self->m_Cursor               = buffer;
            self->m_End                  = end;
            self->m_Alloc                = alloc;
            self->m_Scratch              = scratch;
            self->m_LineNumber           = 1;
            self->m_Lexeme.m_Type        = kJsonLexInvalid;
            self->m_ErrorMessage         = nullptr;
            self->m_ValueSeparatorLexeme = {kJsonLexValueSeparator, {0}};
            self->m_NameSeparatorLexeme  = {kJsonLexNameSeparator, {0}};
            self->m_BeginObjectLexeme    = {kJsonLexBeginObject, {0}};
            self->m_EndObjectLexeme      = {kJsonLexEndObject, {0}};
            self->m_BeginArrayLexeme     = {kJsonLexBeginArray, {0}};
            self->m_EndArrayLexeme       = {kJsonLexEndArray, {0}};
            self->m_NullLexeme           = {kJsonLexNull, {0}};
            self->m_EofLexeme            = {kJsonLexEof, {0}};
            self->m_ErrorLexeme          = {kJsonLexError, {0}};
            self->m_TrueLexeme           = {kJsonLexBoolean, {true}};
            self->m_FalseLexeme          = {kJsonLexBoolean, {false}};
        }

        static JsonLexeme JsonLexerError(JsonLexerState* state, const char* error)
        {
            ASSERT(state->m_ErrorMessage == nullptr);
            int const len         = ascii::strlen(error) + 32;
            state->m_ErrorMessage = state->m_Scratch->AllocateArray<char>(len + 1);
            runes_t  errmsg(state->m_ErrorMessage, state->m_ErrorMessage + len);
            crunes_t fmt("line %d: %s");
            xcore::sprintf(errmsg, fmt, va_t(state->m_LineNumber), va_t(error));
            return state->m_ErrorLexeme;
        }

        static JsonLexeme GetNumberLexeme(JsonLexerState* state)
        {
            char const* start = state->m_Cursor;
            char const* end   = state->m_End;

            JsonLexeme result;
            result.m_Type      = kJsonLexNumber;
            char const* cursor = ParseNumber(start, end, result.m_Number);

            if (result.m_Number.m_Type == kJsonNumber_unknown)
                return JsonLexerError(state, "illegal number");

            state->m_Cursor = cursor;
            return start != cursor ? result : JsonLexerError(state, "bad number");
        }

        static JsonLexeme GetStringLexeme(JsonLexerState* state)
        {
            char const* rptr = state->m_Cursor;

            char c = PeekAsciiChar(rptr, state->m_End);
            if ('\"' != c)
            {
                return JsonLexerError(state, "expecting string delimiter '\"'");
            }
            rptr++;

            char* wend = nullptr;
            char* wptr = (char*)state->m_Alloc->CheckOut(wend);

            JsonLexeme result;
            result.m_Type   = kJsonLexString;
            result.m_String = wptr;

            while (true)
            {
                c = PeekAsciiChar(rptr, state->m_End);
                if (0 == c)
                {
                    // Cancel 'CheckOut'
                    return JsonLexerError(state, "end of file inside string");
                }

                uchar8_t ch = PeekUtf8Char(rptr, state->m_End);
                ASSERT(ch.l >= 1 && ch.l <= 4);
                if ('"' == ch.c)
                {
                    rptr += 1;
                    WriteChar('\0', wptr, wend);
                    break;
                }
                else if ('\\' == ch.c)
                {
                    rptr += 1;
                    c = PeekAsciiChar(rptr, state->m_End);
                    rptr += 1;

                    switch (c)
                    {
                        case '\\': WriteChar('\\', wptr, wend); break;
                        case '"': WriteChar('\"', wptr, wend); break;
                        case '/': WriteChar('/', wptr, wend); break;
                        case 'b': WriteChar('\b', wptr, wend); break;
                        case 'f': WriteChar('\f', wptr, wend); break;
                        case 'n': WriteChar('\n', wptr, wend); break;
                        case 'r': WriteChar('\r', wptr, wend); break;
                        case 't': WriteChar('\t', wptr, wend); break;
                        case 'u':
                        {
                            u32 hex_code = 0;
                            for (s32 i = 0; i < 4; ++i)
                            {
                                c = PeekAsciiChar(rptr, state->m_End);
                                rptr += 1;

                                if (is_hexa(c))
                                {
                                    u32 lc = to_lower(c);
                                    hex_code <<= 4;
                                    if (lc >= 'a' && lc <= 'f')
                                        hex_code |= lc - 'a' + 10;
                                    else
                                        hex_code |= lc - '0';
                                }
                                else
                                {
                                    // Cancel 'CheckOut'
                                    if (0 == c)
                                    {
                                        return JsonLexerError(state, "end of file inside escape code of json string");
                                    }
                                    return JsonLexerError(state, "expected 4 character hex number, e.g. '\\u00004E2D'");
                                }
                            }
                            WriteChar(hex_code, wptr, wend);
                        }

                        default:
                        {
                            // Cancel 'CheckOut'
                            return JsonLexerError(state, "unexpected character in string");
                        }
                    }
                }
                else
                {
                    rptr += ch.l;
                    WriteChar(ch.c, wptr, wend);
                }
            }

            state->m_Alloc->Commit(wptr);

            state->m_Cursor = rptr;
            return result;
        }

        static JsonLexeme GetLiteralLexeme(JsonLexerState* state)
        {
            char const* rptr = state->m_Cursor;
            char const* eptr = rptr;

            s32 kwlen = 0;

            uchar8_t ch = PeekUtf8Char(eptr, state->m_End);
            while (is_alpha(ch.c))
            {
                eptr += ch.l;
                kwlen += 1;
                ch = PeekUtf8Char(eptr, state->m_End);
            }

            if (4 == kwlen)
            {
                if (rptr[0] == 't' && rptr[1] == 'r' && rptr[2] == 'u' && rptr[3] == 'e')
                {
                    state->m_Cursor = eptr;
                    return state->m_TrueLexeme;
                }
                else if (rptr[0] == 'n' && rptr[1] == 'u' && rptr[2] == 'l' && rptr[3] == 'l')
                {
                    state->m_Cursor = eptr;
                    return state->m_NullLexeme;
                }
            }
            else if (5 == kwlen)
            {
                if (rptr[0] == 'f' && rptr[1] == 'a' && rptr[2] == 'l' && rptr[3] == 's' && rptr[4] == 'e')
                {
                    state->m_Cursor = eptr;
                    return state->m_FalseLexeme;
                }
            }

            return JsonLexerError(state, "invalid literal, expected one of false, true or null");
        }

        static char const* SkipWhitespace(JsonLexerState* state)
        {
            uchar8_t ch = PeekUtf8Char(state->m_Cursor, state->m_End);
            while (ch.c != 0)
            {
                if (is_whitespace(ch.c))
                {
                    if ('\n' == ch.c)
                    {
                        ++state->m_LineNumber;
                    }
                    state->m_Cursor += ch.l;
                }
                else
                {
                    break;
                }

                ch = PeekUtf8Char(state->m_Cursor, state->m_End);
            }
            return state->m_Cursor;
        }

        static JsonLexeme JsonLexerFetchNext(JsonLexerState* state)
        {
            char const* p  = SkipWhitespace(state);
            uchar8_t    ch = PeekUtf8Char(p, state->m_End);
            switch (ch.c)
            {
                case '-':
                case '0':
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                case '8':
                case '9': return GetNumberLexeme(state);
                case '"': return GetStringLexeme(state);
                case 't':
                case 'f':
                case 'n': return GetLiteralLexeme(state);
                case '{': state->m_Cursor = p + 1; return state->m_BeginObjectLexeme;
                case '}': state->m_Cursor = p + 1; return state->m_EndObjectLexeme;
                case '[': state->m_Cursor = p + 1; return state->m_BeginArrayLexeme;
                case ']': state->m_Cursor = p + 1; return state->m_EndArrayLexeme;
                case ',': state->m_Cursor = p + 1; return state->m_ValueSeparatorLexeme;
                case ':': state->m_Cursor = p + 1; return state->m_NameSeparatorLexeme;
                case '\0': return state->m_EofLexeme;

                default: // very likely an error
                    return GetLiteralLexeme(state);
            }
        }

        JsonLexeme JsonLexerPeek(JsonLexerState* state)
        {
            if (kJsonLexInvalid == state->m_Lexeme.m_Type)
            {
                state->m_Lexeme = JsonLexerFetchNext(state);
            }

            return state->m_Lexeme;
        }

        JsonLexeme JsonLexerNext(JsonLexerState* state)
        {
            if (kJsonLexInvalid != state->m_Lexeme.m_Type)
            {
                JsonLexeme result      = state->m_Lexeme;
                state->m_Lexeme.m_Type = kJsonLexInvalid;
                return result;
            }

            return JsonLexerFetchNext(state);
        }

        void JsonLexerSkip(JsonLexerState* state)
        {
            if (kJsonLexInvalid != state->m_Lexeme.m_Type)
            {
                state->m_Lexeme.m_Type = kJsonLexInvalid;
                return;
            }

            JsonLexerNext(state);
        }

        bool JsonLexerExpect(JsonLexerState* state, JsonLexemeType type, JsonLexeme* out)
        {
            JsonLexeme l = JsonLexerNext(state);
            if (l.m_Type == type)
            {
                if (out)
                    *out = l;
                return true;
            }

            return false;
        }

    } // namespace json
} // namespace xcore
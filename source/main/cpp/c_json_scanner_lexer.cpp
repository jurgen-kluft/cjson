#include "cbase/c_allocator.h"
#include "cbase/c_context.h"
#include "cbase/c_printf.h"
#include "cbase/c_runes.h"
#include "cjson/c_json_utils.h"
#include "cjson/c_json_allocator.h"

#include "cjson/c_json_scanner.h"
#include "cjson/c_json_scanner_lexer.h"

namespace ncore
{
    namespace njson
    {
        namespace nscanner
        {
            void JsonLexerStateInit(JsonLexerState* self, char const* buffer, char const* end, JsonAllocator* alloc)
            {
                self->m_Cursor               = buffer;
                self->m_End                  = end;
                self->m_Alloc                = alloc;
                self->m_LineNumber           = 1;
                self->m_Lexeme.m_Type        = kJsonLexInvalid;
                self->m_ErrorMessage         = nullptr;
                self->m_ValueSeparatorLexeme = JsonLexeme(kJsonLexValueSeparator);
                self->m_NameSeparatorLexeme  = JsonLexeme(kJsonLexNameSeparator);
                self->m_BeginObjectLexeme    = JsonLexeme(kJsonLexBeginObject);
                self->m_EndObjectLexeme      = JsonLexeme(kJsonLexEndObject);
                self->m_BeginArrayLexeme     = JsonLexeme(kJsonLexBeginArray);
                self->m_EndArrayLexeme       = JsonLexeme(kJsonLexEndArray);
                self->m_NullLexeme           = JsonLexeme(kJsonLexNull);
                self->m_EofLexeme            = JsonLexeme(kJsonLexEof);
                self->m_ErrorLexeme          = JsonLexeme(kJsonLexError);
            }

            static JsonLexeme JsonLexerError(JsonLexerState* state, const char* error)
            {
                ASSERT(state->m_ErrorMessage == nullptr);
                int const len         = ascii::strlen(error) + 32;
                state->m_ErrorMessage = state->m_Alloc->AllocateArray<char>(len + 1);
                runes_t  errmsg       = ascii::make_runes(state->m_ErrorMessage, state->m_ErrorMessage + len);
                crunes_t fmt          = ascii::make_crunes("line %d: %s");
                sprintf(errmsg, fmt, va_t(state->m_LineNumber), va_t(error));
                return state->m_ErrorLexeme;
            }

            static JsonLexeme GetNumberLexeme(JsonLexerState* state)
            {
                char const* str = state->m_Cursor;
                char const* end = state->m_Cursor;

                // Scan number, which means we need to just look for the following characters:
                // - , whitespace, ], }
                while (true)
                {
                    uchar8_t ch = PeekUtf8Char(end, state->m_End);
                    if (ch.c == 0 || nrunes::is_whitespace(ch.c) || ch.c == ',' || ch.c == ']' || ch.c == '}')
                    {
                        break;
                    }
                    end += ch.l;
                }

                JsonLexeme result;
                result.m_Type = kJsonLexNumber;
                result.m_Str  = str;
                result.m_Len  = (u32)(end - str);

                state->m_Cursor = end;
                return result;
            }

            static JsonLexeme GetStringLexeme(JsonLexerState* state)
            {
                char const* rptr = state->m_Cursor;
                char        c    = PeekAsciiChar(rptr, state->m_End);
                if ('\"' != c)
                {
                    return JsonLexerError(state, "expecting string delimiter '\"'");
                }
                rptr++;

                JsonLexeme result;
                result.m_Type = kJsonLexString;
                result.m_Str  = rptr;
                result.m_Len  = 0;

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
                    if (ch.l == 1 && '"' == ch.c)
                    {
                        rptr += 1;
                        break;
                    }
                    else if ('\\' == ch.c)
                    {
                        rptr += 2;
                    }
                    else
                    {
                        rptr += ch.l;
                    }
                }
                state->m_Cursor = rptr;

                result.m_Len    = (u32)(rptr - result.m_Str - 1); // exclude ending quote
                return result;
            }

            static JsonLexeme GetLiteralLexeme(JsonLexerState* state)
            {
                char const* rptr = state->m_Cursor;
                char const* eptr = rptr;

                s32 kwlen = 0;

                uchar8_t ch = PeekUtf8Char(eptr, state->m_End);
                while (nrunes::is_alpha(ch.c))
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
                        JsonLexeme lexeme(kJsonLexBoolean);
                        lexeme.m_Len = 4;
                        lexeme.m_Str = (char*)rptr;
                        return lexeme;
                    }
                    else if (rptr[0] == 'n' && rptr[1] == 'u' && rptr[2] == 'l' && rptr[3] == 'l')
                    {
                        state->m_Cursor = eptr;
                        JsonLexeme lexeme(kJsonLexNull);
                        lexeme.m_Len = 4;
                        lexeme.m_Str = (char*)rptr;
                        return lexeme;
                    }
                }
                else if (5 == kwlen)
                {
                    if (rptr[0] == 'f' && rptr[1] == 'a' && rptr[2] == 'l' && rptr[3] == 's' && rptr[4] == 'e')
                    {
                        state->m_Cursor = eptr;
                        JsonLexeme lexeme(kJsonLexBoolean);
                        lexeme.m_Len = 5;
                        lexeme.m_Str = (char*)rptr;
                        return lexeme;
                    }
                }

                return JsonLexerError(state, "invalid literal, expected one of false, true or null");
            }

            static char const* SkipWhitespace(JsonLexerState* state)
            {
                uchar8_t ch = PeekUtf8Char(state->m_Cursor, state->m_End);
                while (ch.c != 0)
                {
                    if (nrunes::is_whitespace(ch.c))
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
        } // namespace nscanner
    } // namespace njson
} // namespace ncore

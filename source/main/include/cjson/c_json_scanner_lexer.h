#ifndef __CJSON_JSON_SCANNER_LEXER_H__
#define __CJSON_JSON_SCANNER_LEXER_H__
#include "ccore/c_target.h"
#ifdef USE_PRAGMA_ONCE
#    pragma once
#endif

#include "cjson/c_json_utils.h"

namespace ncore
{
    namespace njson
    {
        struct JsonAllocator;

        namespace nscanner
        {
            enum JsonLexemeType
            {
                kJsonLexError          = -1,
                kJsonLexInvalid        = 0,
                kJsonLexString         = 1,
                kJsonLexValueSeparator = 3,
                kJsonLexNameSeparator  = 4,
                kJsonLexBeginObject    = 5,
                kJsonLexBeginArray     = 6,
                kJsonLexEndObject      = 7,
                kJsonLexEndArray       = 8,
                kJsonLexBoolean        = 9,
                kJsonLexNull           = 10,
                kJsonLexEof            = 11,
                kJsonLexNumber         = 12,
            };

            struct JsonLexeme
            {
                JsonLexeme()
                    : m_Type(kJsonLexInvalid)
                    , m_Len(0)
                    , m_Str(nullptr)
                {
                }
                JsonLexeme(JsonLexemeType type)
                    : m_Type(type)
                    , m_Len(0)
                    , m_Str(nullptr)
                {
                }

                JsonLexemeType m_Type;
                u32            m_Len;
                const char*    m_Str;
            };

            struct JsonLexerState
            {
                const char*    m_Cursor;
                char const*    m_End;
                JsonAllocator* m_Alloc;
                s32            m_LineNumber;
                JsonLexeme     m_Lexeme;
                char*          m_ErrorMessage;
                JsonLexeme     m_ValueSeparatorLexeme;
                JsonLexeme     m_NameSeparatorLexeme;
                JsonLexeme     m_BeginObjectLexeme;
                JsonLexeme     m_EndObjectLexeme;
                JsonLexeme     m_BeginArrayLexeme;
                JsonLexeme     m_EndArrayLexeme;
                JsonLexeme     m_NullLexeme;
                JsonLexeme     m_EofLexeme;
                JsonLexeme     m_ErrorLexeme;
            };

            void       JsonLexerStateInit(JsonLexerState* self, char const* buffer, char const* end, JsonAllocator* alloc);
            bool       JsonLexerExpect(JsonLexerState* state, JsonLexemeType type, JsonLexeme* out = nullptr);
            JsonLexeme JsonLexerPeek(JsonLexerState* state);
            void       JsonLexerSkip(JsonLexerState* state);
            JsonLexeme JsonLexerNext(JsonLexerState* state);
        } // namespace nscanner
    } // namespace njson
} // namespace ncore

#endif // __CJSON_JSON_SCANNER_LEXER_H__

#ifndef __CJSON_JSON_LEXER_H__
#define __CJSON_JSON_LEXER_H__
#include "ccore/c_target.h"
#ifdef USE_PRAGMA_ONCE
#    pragma once
#endif

#include "cjson/c_json_utils.h"

namespace ncore
{
    namespace njson
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

        struct JsonString
        {
            u32   m_Len;
            char* m_Str;
        };

        struct JsonLexeme
        {
            JsonLexeme() : m_Type(kJsonLexInvalid), m_Number() {}
            JsonLexeme(JsonLexemeType type) : m_Type(type), m_Number() {}
            JsonLexeme(JsonLexemeType type, JsonNumber number) : m_Type(type), m_Number(number) {}

            JsonLexemeType m_Type;
            union
            {
                JsonNumber m_Number;
                JsonString m_String;
            };
        };

        struct JsonAllocator;

        struct JsonLexerState
        {
            const char*    m_Cursor;
            char const*    m_End;
            JsonAllocator* m_Alloc;
            JsonAllocator* m_Scratch;
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
            JsonLexeme     m_TrueLexeme;
            JsonLexeme     m_FalseLexeme;
        };

        void       JsonLexerStateInit(JsonLexerState* self, char const* buffer, char const* end, JsonAllocator* alloc, JsonAllocator* scratch);
        bool       JsonLexerExpect(JsonLexerState* state, JsonLexemeType type, JsonLexeme* out = nullptr);
        JsonLexeme JsonLexerPeek(JsonLexerState* state);
        void       JsonLexerSkip(JsonLexerState* state);
        JsonLexeme JsonLexerNext(JsonLexerState* state);

    } // namespace njson
} // namespace ncore

#endif // __CJSON_JSON_LEXER_H__

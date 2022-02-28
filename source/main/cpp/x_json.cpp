#include "xbase/x_allocator.h"
#include "xbase/x_context.h"
#include "xbase/x_printf.h"
#include "xbase/x_runes.h"
#include "xjson/x_json.h"
#include "xjson/x_json_utils.h"
#include "xjson/x_json_allocator.h"

namespace xcore
{
    namespace json
    {
        const JsonValue* JsonValue::Find(const char* key) const
        {
            const JsonObjectValue* obj = AsObject();
            for (int i = 0, count = obj->m_Count; i < count; ++i)
            {
                const char* cmp_key = key;
                const char* obj_key = obj->m_Names[i];
                while (*cmp_key != 0 && *obj_key != 0 && *cmp_key == *obj_key)
                    ++cmp_key, ++obj_key;
                if (*cmp_key == 0 && *obj_key == 0)
                    return obj->m_Values[i];
            }
            return nullptr;
        }

        enum JsonLexemeType
        {
            kJsonLexString,
            kJsonLexNumber,
            kJsonLexValueSeparator,
            kJsonLexNameSeparator,
            kJsonLexBeginObject,
            kJsonLexBeginArray,
            kJsonLexEndObject,
            kJsonLexEndArray,
            kJsonLexBoolean,
            kJsonLexNull,
            kJsonLexEof,
            kJsonLexError,
            kJsonLexInvalid
        };

        struct JsonLexeme
        {
            JsonLexemeType m_Type;
            union
            {
                bool   m_Boolean;
                double m_Number;
                char*  m_String;
            };
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
            JsonLexeme     m_TrueLexeme;
            JsonLexeme     m_FalseLexeme;
        };

        static void JsonLexerStateInit(JsonLexerState* self, char const* buffer, char const* end, JsonAllocator* alloc)
        {
            self->m_Cursor               = buffer;
            self->m_End                  = end;
            self->m_Alloc                = alloc;
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

        static char const* SkipWhitespace(JsonLexerState* state)
        {
            uchar8_t ch = PeekChar(state->m_Cursor, state->m_End);
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

                ch = PeekChar(state->m_Cursor, state->m_End);
            }
            return state->m_Cursor;
        }

        static JsonLexeme JsonLexerError(JsonLexerState* state, const char* error)
        {
            ASSERT(state->m_ErrorMessage == nullptr);
            int const len         = ascii::strlen(error) + 32;
            state->m_ErrorMessage = state->m_Alloc->AllocateArray<char>(len + 1);
            runes_t  errmsg(state->m_ErrorMessage, state->m_ErrorMessage + len);
            crunes_t fmt("line %d: %s");
            xcore::sprintf(errmsg, fmt, va_t(state->m_LineNumber), va_t(error));
            return state->m_ErrorLexeme;
        }

        static JsonLexeme GetNumberLexeme(JsonLexerState* state)
        {
            char const* start = state->m_Cursor;
            char const* end   = state->m_End;

            f64         number;
            char const* cursor = ParseNumber(start, end, &number);

            JsonLexeme result;
            result.m_Type   = kJsonLexNumber;
            result.m_Number = number;
            state->m_Cursor = cursor;
            return start != cursor ? result : JsonLexerError(state, "bad number");
        }

        static JsonLexeme GetStringLexeme(JsonLexerState* state)
        {
            char const* rptr = state->m_Cursor;

            uchar8_t ch = PeekChar(rptr, state->m_End);
            ASSERT(ch.c == '\"');
            rptr += ch.l;

            char* wend = nullptr;
            char* wptr = (char*)state->m_Alloc->CheckOut(wend);

            JsonLexeme result;
            result.m_Type   = kJsonLexString;
            result.m_String = wptr;

            while (true)
            {
                ch = PeekChar(rptr, state->m_End);

                if (0 == ch.c)
                {
                    // Cancel 'CheckOut'
                    return JsonLexerError(state, "end of file inside string");
                }

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
                    ch = PeekChar(rptr, state->m_End);
                    rptr += 1;

                    switch (ch.c)
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
                                ch = PeekChar(rptr, state->m_End);
                                rptr += 1;

                                ASSERT(ch.l == 1);
                                if (is_hexa(ch.c))
                                {
                                    u32 lc = to_lower(ch.c);
                                    hex_code <<= 4;
                                    if (lc >= 'a' && lc <= 'f')
                                        hex_code |= lc - 'a' + 10;
                                    else
                                        hex_code |= lc - '0';
                                }
                                else
                                {
                                    if (0 == ch.c)
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

            uchar8_t ch = PeekChar(eptr, state->m_End);
            while (is_alpha(ch.c))
            {
                eptr += ch.l;
                kwlen += 1;
                ch = PeekChar(eptr, state->m_End);
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

        static JsonLexeme JsonLexerFetchNext(JsonLexerState* state)
        {
            char const* p  = SkipWhitespace(state);
            uchar8_t    ch = PeekChar(p, state->m_End);
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

        static JsonLexeme JsonLexerPeek(JsonLexerState* state)
        {
            if (kJsonLexInvalid == state->m_Lexeme.m_Type)
            {
                state->m_Lexeme = JsonLexerFetchNext(state);
            }

            return state->m_Lexeme;
        }

        static JsonLexeme JsonLexerNext(JsonLexerState* state)
        {
            if (kJsonLexInvalid != state->m_Lexeme.m_Type)
            {
                JsonLexeme result      = state->m_Lexeme;
                state->m_Lexeme.m_Type = kJsonLexInvalid;
                return result;
            }

            return JsonLexerFetchNext(state);
        }

        static void JsonLexerSkip(JsonLexerState* state)
        {
            if (kJsonLexInvalid != state->m_Lexeme.m_Type)
            {
                state->m_Lexeme.m_Type = kJsonLexInvalid;
                return;
            }

            JsonLexerNext(state);
        }

        static bool JsonLexerExpect(JsonLexerState* state, JsonLexemeType type, JsonLexeme* out = nullptr)
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

        struct JsonState
        {
            JsonLexerState    m_Lexer;
            char*             m_ErrorMessage;
            JsonAllocator*    m_Allocator;
            JsonAllocator*    m_Scratch;
            int               m_NumberOfObjects;
            int               m_NumberOfNumbers;
            int               m_NumberOfStrings;
            int               m_NumberOfArrays;
            int               m_NumberOfBooleans;
            JsonBooleanValue* m_TrueValue;
            JsonBooleanValue* m_FalseValue;
            JsonValue*        m_NullValue;
        };

        static void JsonStateInit(JsonState* state, JsonAllocator* alloc, JsonAllocator* scratch, char const* buffer, char const* end)
        {
            JsonLexerStateInit(&state->m_Lexer, buffer, end, alloc);
            state->m_ErrorMessage          = nullptr;
            state->m_Allocator             = alloc;
            state->m_Scratch               = scratch;
            state->m_NumberOfObjects       = 0;
            state->m_NumberOfNumbers       = 0;
            state->m_NumberOfStrings       = 0;
            state->m_NumberOfArrays        = 0;
            state->m_NumberOfBooleans      = 2;
            state->m_TrueValue             = alloc->Allocate<JsonBooleanValue>();
            state->m_TrueValue->m_Type     = JsonValue::kBoolean;
            state->m_TrueValue->m_Boolean  = true;
            state->m_FalseValue            = alloc->Allocate<JsonBooleanValue>();
            state->m_FalseValue->m_Type    = JsonValue::kBoolean;
            state->m_FalseValue->m_Boolean = false;
            state->m_NullValue             = alloc->Allocate<JsonValue>();
            state->m_NullValue->m_Type     = JsonValue::kNull;
        }

        static JsonValue* JsonError(JsonState* state, const char* error)
        {
            state->m_ErrorMessage = state->m_Allocator->AllocateArray<char>(1024);
            runes_t  errmsg(state->m_ErrorMessage, state->m_ErrorMessage + 1024 - 1);
            crunes_t fmt("line %d: %s");
            xcore::sprintf(errmsg, fmt, va_t(state->m_Lexer.m_LineNumber), va_t(error));
            return nullptr;
        }

        static const JsonValue* JsonParseValue(JsonState* json_state);

        static const JsonValue* JsonParseObject(JsonState* json_state)
        {
            JsonLexerState* lexer = &json_state->m_Lexer;

            if (!JsonLexerExpect(lexer, kJsonLexBeginObject))
                return JsonError(json_state, "expected '{'");

            JsonAllocatorScope scratch_scope(json_state->m_Scratch);

            bool seen_value = false;
            bool seen_comma = false;

            struct KvPair
            {
                const char*      m_Key;
                const JsonValue* m_Value;
                KvPair*          m_Next;
            };

            struct KvPairList
            {
                JsonAllocator* m_Scratch;
                KvPair*        m_Head;
                KvPair*        m_Tail;
                s32            m_Count;

                void Init(JsonAllocator* scratch)
                {
                    m_Scratch = scratch;
                    m_Head    = nullptr;
                    m_Tail    = nullptr;
                    m_Count   = 0;
                }

                void Add(const char* key, const JsonValue* value)
                {
                    if (1 == ++m_Count)
                    {
                        m_Head = m_Tail = m_Scratch->Allocate<KvPair>();
                        m_Head->m_Key   = key;
                        m_Head->m_Value = value;
                        m_Head->m_Next  = nullptr;
                    }
                    else
                    {
                        KvPair* tail    = m_Tail;
                        m_Tail          = m_Scratch->Allocate<KvPair>();
                        m_Tail->m_Key   = key;
                        m_Tail->m_Value = value;
                        m_Tail->m_Next  = nullptr;
                        tail->m_Next    = m_Tail;
                    }
                }
            };

            KvPairList kv_pairs;
            kv_pairs.Init(json_state->m_Scratch);

            bool done = false;
            while (!done)
            {
                JsonLexeme l = JsonLexerNext(lexer);

                switch (l.m_Type)
                {
                    case kJsonLexString:
                    {
                        if (seen_value && !seen_comma)
                            return JsonError(json_state, "expected ','");

                        if (!JsonLexerExpect(lexer, kJsonLexNameSeparator))
                            return JsonError(json_state, "expected ':'");

                        const JsonValue* value = JsonParseValue(json_state);

                        if (value == nullptr)
                            return nullptr;

                        kv_pairs.Add(l.m_String, value);

                        seen_value = true;
                        seen_comma = false;
                    }
                    break;

                    case kJsonLexEndObject: done = true; break;

                    case kJsonLexValueSeparator:
                    {
                        if (!seen_value)
                            return JsonError(json_state, "expected key name");

                        if (seen_comma)
                            return JsonError(json_state, "duplicate comma");

                        seen_value = false;
                        seen_comma = true;
                        break;
                    }

                    default: return JsonError(json_state, "expected object to continue");
                }
            }

            JsonAllocator* alloc = json_state->m_Allocator;

            s32               count  = kv_pairs.m_Count;
            const char**      names  = alloc->AllocateArray<const char*>(kv_pairs.m_Count);
            const JsonValue** values = alloc->AllocateArray<const JsonValue*>(kv_pairs.m_Count);

            s32 index = 0;
            for (KvPair* p = kv_pairs.m_Head; p; p = p->m_Next, ++index)
            {
                names[index]  = p->m_Key;
                values[index] = p->m_Value;
            }

            json_state->m_NumberOfObjects += 1;
            JsonObjectValue* result = alloc->Allocate<JsonObjectValue>();
            result->m_Type          = JsonValue::kObject;
            result->m_Count         = count;
            result->m_Names         = names;
            result->m_Values        = values;

            return result;
        }

        static const JsonValue* JsonParseArray(JsonState* json_state)
        {
            JsonLexerState* lexer = &json_state->m_Lexer;

            if (!JsonLexerExpect(lexer, kJsonLexBeginArray))
                return JsonError(json_state, "expected '['");

            JsonAllocatorScope scratch_scope(json_state->m_Scratch);

            struct ListElem
            {
                const JsonValue* m_Value;
                ListElem*        m_Next;
            };

            struct ValueList
            {
                JsonAllocator* m_Scratch;
                ListElem*      m_Head;
                ListElem*      m_Tail;
                s32            m_Count;

                void Init(JsonAllocator* scratch)
                {
                    m_Scratch = scratch;
                    m_Head    = nullptr;
                    m_Tail    = nullptr;
                    m_Count   = 0;
                }

                void Add(const JsonValue* value)
                {
                    if (1 == ++m_Count)
                    {
                        m_Head = m_Tail = m_Scratch->Allocate<ListElem>();
                        m_Head->m_Value = value;
                        m_Head->m_Next  = nullptr;
                    }
                    else
                    {
                        ListElem* tail  = m_Tail;
                        m_Tail          = m_Scratch->Allocate<ListElem>();
                        m_Tail->m_Value = value;
                        m_Tail->m_Next  = nullptr;
                        tail->m_Next    = m_Tail;
                    }
                }
            };

            ValueList value_list;

            value_list.Init(json_state->m_Scratch);

            for (;;)
            {
                JsonLexeme l = JsonLexerPeek(lexer);

                if (kJsonLexEndArray == l.m_Type)
                {
                    JsonLexerSkip(lexer);
                    break;
                }

                if (value_list.m_Count > 0)
                {
                    if (kJsonLexValueSeparator != l.m_Type)
                    {
                        return JsonError(json_state, "expected ','");
                    }

                    JsonLexerSkip(lexer);
                }

                const JsonValue* value = JsonParseValue(json_state);
                if (!value)
                    return nullptr;

                value_list.Add(value);
            }

            JsonAllocator* alloc = json_state->m_Allocator;

            s32               count  = value_list.m_Count;
            const JsonValue** values = alloc->AllocateArray<const JsonValue*>(count);

            s32 index = 0;
            for (ListElem* p = value_list.m_Head; p; p = p->m_Next, ++index)
            {
                values[index] = p->m_Value;
            }

            json_state->m_NumberOfArrays += 1;
            JsonArrayValue* result = alloc->Allocate<JsonArrayValue>();
            result->m_Type         = JsonValue::kArray;
            result->m_Count        = count;
            result->m_Values       = values;

            return result;
        }

        bool JsonValue::GetColor(u32& out_color) const
        {
            if (m_Type != kString)
                return false;

            const char* str = this->AsString()->m_String;
            const char* end = str + 9;

            uchar8_t ch = PeekChar(str, end);
            if (ch.c != '#')
                return false;

            str += ch.l;
            ch = PeekChar(str, end);

            u8 color[4] = {0, 0, 0, 0xff};
            s8 c        = 0;
            s8 n        = 0;
            u8 v        = 0;
            while (ch.c != '\0')
            {
                if (ch.c >= '0' && ch.c <= '9')
                {
                    if (n == 0)
                    {
                        v = (ch.c - '0') << 4;
                        n = 1;
                    }
                    else if (n == 1)
                    {
                        v |= (ch.c - '0');
                        color[c++] = v;
                        n          = 0;
                    }
                }
                else
                {
                    return false;
                }

                str += ch.l;
            }

            if (c != 4)
                return false;

            out_color = (color[0] << 24) | (color[1] << 16) | (color[2] << 8) | color[3];
            return true;
        }

        static const JsonValue* JsonParseValue(JsonState* json_state)
        {
            const JsonValue* result = nullptr;
            JsonLexeme       l      = JsonLexerPeek(&json_state->m_Lexer);
            switch (l.m_Type)
            {
                case kJsonLexBeginObject: result = JsonParseObject(json_state); break;
                case kJsonLexBeginArray: result = JsonParseArray(json_state); break;

                case kJsonLexString:
                {
                    json_state->m_NumberOfStrings += 1;
                    JsonStringValue* sv = json_state->m_Allocator->Allocate<JsonStringValue>();
                    sv->m_Type          = JsonValue::kString;
                    sv->m_String        = l.m_String;
                    result              = sv;
                    JsonLexerSkip(&json_state->m_Lexer);
                    break;
                }

                case kJsonLexNumber:
                {
                    json_state->m_NumberOfNumbers += 1;
                    JsonNumberValue* nv = json_state->m_Allocator->Allocate<JsonNumberValue>();
                    nv->m_Type          = JsonValue::kNumber;
                    nv->m_Number        = l.m_Number;
                    result              = nv;
                    JsonLexerSkip(&json_state->m_Lexer);
                    break;
                }

                case kJsonLexBoolean:
                    result = l.m_Boolean ? json_state->m_TrueValue : json_state->m_FalseValue;
                    JsonLexerSkip(&json_state->m_Lexer);
                    break;

                case kJsonLexNull:
                    result = json_state->m_NullValue;
                    JsonLexerSkip(&json_state->m_Lexer);
                    break;

                default: result = JsonError(json_state, "invalid document"); break;
            }

            return result;
        }

        const JsonValue* JsonParse(char const* str, char const* end, JsonAllocator* allocator, JsonAllocator* scratch, char const*& error_message)
        {
            JsonState* json_state = scratch->Allocate<JsonState>();
            JsonStateInit(json_state, allocator, scratch, str, end);

            const JsonValue* root = JsonParseValue(json_state);
            if (root && !JsonLexerExpect(&json_state->m_Lexer, kJsonLexEof))
            {
                root = JsonError(json_state, "data after document");
            }

            // Mainly to reduce the duplication of keys (strings) but in some way also string values.
            // We could benefit from an additional 2 allocators that are used for:
            // - [Hash 32-bit, String Offset 32-bit] pairs (sorted)
            // - Strings (UTF-8 strings)
            // The user could even pre-warm these with known strings and even reuse them between different JSON documents.

            error_message = nullptr;
            if (!root)
            {
                s32 const len    = ascii::strlen(json_state->m_ErrorMessage);
                char*     errmsg = scratch->AllocateArray<char>(len + 1);
                x_memcopy(errmsg, json_state->m_ErrorMessage, len);
                errmsg[len]   = '\0';
                error_message = errmsg;
            }

            return root;
        }

    } // namespace json
} // namespace xcore
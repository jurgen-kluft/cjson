#include "xbase/x_allocator.h"
#include "xbase/x_context.h"
#include "xbase/x_printf.h"
#include "xbase/x_runes.h"
#include "xjson/x_json.h"
#include "xjson/x_json_lexer.h"
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
            JsonLexerStateInit(&state->m_Lexer, buffer, end, alloc, scratch);
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
            state->m_ErrorMessage = state->m_Scratch->AllocateArray<char>(1024);
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
                return JsonError(json_state, "missing '{'");

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
                            return JsonError(json_state, "missing ','");

                        if (!JsonLexerExpect(lexer, kJsonLexNameSeparator))
                            return JsonError(json_state, "missing ':'");

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
                inline ListElem()
                    : m_Value(nullptr)
                    , m_Next(nullptr)
                {
                }
                const JsonValue* m_Value;
                ListElem*        m_Next;
            };

            struct ValueList
            {
                JsonAllocator* m_Scratch;
                ListElem       m_Head;
                ListElem*      m_Tail;
                s32            m_Count;

                void Init(JsonAllocator* scratch)
                {
                    m_Scratch = scratch;
                    m_Tail    = &m_Head;
                    m_Count   = 0;
                }

                void Add(const JsonValue* value)
                {
                    ListElem* elem = m_Scratch->Allocate<ListElem>();
                    elem->m_Next   = nullptr;
                    elem->m_Value  = value;
                    m_Tail->m_Next = elem;
                    m_Tail         = elem;
                    m_Count += 1;
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
            for (ListElem* p = value_list.m_Head.m_Next; p; p = p->m_Next, ++index)
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
                
            const JsonStringValue* stringValue = static_cast<const JsonStringValue*>(this);
            const char* str = stringValue->m_String;
            const char* end = str + 9;

            char c = PeekAsciiChar(str, end);
            if (c != '#')
                return false;
            str++;

            c = PeekAsciiChar(str, end);

            u8 ca[4] = {0, 0, 0, 0xff};
            s8 ci    = 0;
            s8 n     = 0;
            u8 v     = 0;
            while (c != '\0')
            {
                if (c >= '0' && c <= '9')
                {
                    c = c - '0';
                    switch (n)
                    {
                        case 0: v = c << 4; break;
                        case 1: ca[ci++] = v | c; break;
                    }
                    n = 1 - n;
                }
                else
                {
                    return false;
                }

                str++;
                c = PeekAsciiChar(str, end);
            }

            if (ci != 4)
                return false;

            out_color = (ca[0] << 24) | (ca[1] << 16) | (ca[2] << 8) | ca[3];
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
                    nv->m_Number        = JsonNumberAsFloat64(l.m_Number);
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
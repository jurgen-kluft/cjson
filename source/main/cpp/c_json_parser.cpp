#include "cbase/c_allocator.h"
#include "cbase/c_context.h"
#include "cbase/c_memory.h"
#include "cbase/c_printf.h"
#include "cbase/c_runes.h"
#include "cjson/c_json_parser.h"
#include "cjson/c_json_parser_lexer.h"
#include "cjson/c_json_utils.h"
#include "cjson/c_json_allocator.h"

namespace ncore
{
    namespace njson
    {
        const JsonValue* JsonValue::Find(const char* key) const
        {
            const JsonObjectValue* obj         = AsObject();
            const JsonNamedValue*  named_value = obj->m_LinkedList;
            for (int i = 0, count = obj->m_Count; i < count; ++i)
            {
                const char* cmp_key = key;
                const char* obj_key = named_value->m_Name;
                while (*cmp_key != 0 && *obj_key != 0 && *cmp_key == *obj_key)
                    ++cmp_key, ++obj_key;
                if (*cmp_key == 0 && *obj_key == 0)
                    return named_value->m_Value;
                named_value = named_value->m_Next;
            }
            return nullptr;
        }

        struct JsonState
        {
            JsonLexerState m_Lexer;
            char*          m_ErrorMessage;
            JsonAllocator* m_Allocator;
            JsonAllocator* m_Scratch;
            int            m_NumberOfObjects;
            int            m_NumberOfNumbers;
            int            m_NumberOfStrings;
            int            m_NumberOfArrays;
            int            m_NumberOfBooleans;
            JsonValue*     m_TrueValue;
            JsonValue*     m_FalseValue;
            JsonValue*     m_NullValue;
        };

        static void JsonStateInit(JsonState* state, JsonAllocator* alloc, JsonAllocator* scratch, char const* buffer, char const* end)
        {
            JsonLexerStateInit(&state->m_Lexer, buffer, end, alloc, scratch);
            state->m_ErrorMessage                            = nullptr;
            state->m_Allocator                               = alloc;
            state->m_Scratch                                 = scratch;
            state->m_NumberOfObjects                         = 0;
            state->m_NumberOfNumbers                         = 0;
            state->m_NumberOfStrings                         = 0;
            state->m_NumberOfArrays                          = 0;
            state->m_NumberOfBooleans                        = 2;
            state->m_TrueValue                               = alloc->Allocate<JsonValue>();
            state->m_TrueValue->m_Type                       = JsonValue::kBoolean;
            state->m_TrueValue->m_Value.m_Boolean.m_Boolean  = true;
            state->m_FalseValue                              = alloc->Allocate<JsonValue>();
            state->m_FalseValue->m_Type                      = JsonValue::kBoolean;
            state->m_FalseValue->m_Value.m_Boolean.m_Boolean = false;
            state->m_NullValue                               = alloc->Allocate<JsonValue>();
            state->m_NullValue->m_Type                       = JsonValue::kNull;
        }

        static JsonValue* JsonError(JsonState* state, const char* error)
        {
            state->m_ErrorMessage = state->m_Scratch->AllocateArray<char>(1024);
            runes_t  errmsg       = ascii::make_runes(state->m_ErrorMessage, state->m_ErrorMessage + 1024 - 1);
            crunes_t fmt          = ascii::make_crunes("line %d: %s");
            ncore::sprintf(errmsg, fmt, va_t(state->m_Lexer.m_LineNumber), va_t(error));
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
                const char*      m_KeyEnd;
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

                void Add(const char* key, const char* key_end, const JsonValue* value)
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

            // KvPairList kv_pairs;
            // kv_pairs.Init(json_state->m_Scratch);

            JsonAllocator* alloc  = json_state->m_Allocator;
            JsonValue*     result = alloc->Allocate<JsonValue>();
            result->m_Type        = JsonValue::kObject;

            result->m_Value.m_Object.m_Count      = 0;
            result->m_Value.m_Object.m_LinkedList = nullptr;

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

                        JsonNamedValue* named_value           = alloc->Allocate<JsonNamedValue>();
                        named_value->m_Name                   = l.m_String.m_Str;
                        named_value->m_Value                  = value;
                        named_value->m_Next                   = result->m_Value.m_Object.m_LinkedList;
                        result->m_Value.m_Object.m_LinkedList = named_value;
                        result->m_Value.m_Object.m_Count += 1;

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

            json_state->m_NumberOfObjects += 1;
            return result;
        }

        static const JsonValue* JsonParseArray(JsonState* json_state)
        {
            JsonLexerState* lexer = &json_state->m_Lexer;

            if (!JsonLexerExpect(lexer, kJsonLexBeginArray))
                return JsonError(json_state, "expected '['");

            JsonAllocator* alloc = json_state->m_Allocator;

            JsonValue* result                    = alloc->Allocate<JsonValue>();
            result->m_Type                       = JsonValue::kArray;
            result->m_Value.m_Array.m_Count      = 0;
            result->m_Value.m_Array.m_LinkedList = nullptr;

            i32              count = 0;
            JsonLinkedValue* head  = nullptr;
            JsonLinkedValue* tail  = nullptr;
            for (;;)
            {
                JsonLexeme l = JsonLexerPeek(lexer);

                if (kJsonLexEndArray == l.m_Type)
                {
                    JsonLexerSkip(lexer);
                    break;
                }

                if (count > 0)
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

                JsonLinkedValue* linked_value = alloc->Allocate<JsonLinkedValue>();
                linked_value->m_Value         = value;
                linked_value->m_Next          = nullptr;
                if (count == 0)
                {
                    head = tail = linked_value;
                }
                else
                {
                    tail->m_Next = linked_value;
                    tail         = linked_value;
                }
                count += 1;
            }

            result->m_Value.m_Array.m_Count      = count;
            result->m_Value.m_Array.m_LinkedList = head;

            json_state->m_NumberOfArrays += 1;

            return result;
        }

        bool JsonValue::GetColor(u32& out_color) const
        {
            if (m_Type != kString)
                return false;

            const char* str = m_Value.m_String.m_String;
            const char* end = m_Value.m_String.m_End;

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
                    JsonValue* sv                 = json_state->m_Allocator->Allocate<JsonValue>();
                    sv->m_Type                    = JsonValue::kString;
                    sv->m_Value.m_String.m_String = l.m_String.m_Str;
                    sv->m_Value.m_String.m_End    = l.m_String.m_Str + l.m_String.m_Len;
                    result                        = sv;
                    JsonLexerSkip(&json_state->m_Lexer);
                    break;
                }

                case kJsonLexNumber:
                {
                    json_state->m_NumberOfNumbers += 1;
                    JsonValue* nv                     = json_state->m_Allocator->Allocate<JsonValue>();
                    nv->m_Type                        = JsonValue::kNumber;
                    nv->m_Value.m_Number.m_NumberType = l.m_Number.m_Type;
                    nv->m_Value.m_Number.m_F64        = JsonNumberAsFloat64(l.m_Number);
                    result                            = nv;
                    JsonLexerSkip(&json_state->m_Lexer);
                    break;
                }

                case kJsonLexBoolean:
                    result = l.m_Number.m_S64 != 0 ? json_state->m_TrueValue : json_state->m_FalseValue;
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

        const JsonValue* Parse(char const* str, char const* end, JsonAllocator* allocator, JsonAllocator* scratch, char const*& error_message)
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
                nmem::memcpy(errmsg, json_state->m_ErrorMessage, len);
                errmsg[len]   = '\0';
                error_message = errmsg;
            }

            return root;
        }

    } // namespace njson
} // namespace ncore

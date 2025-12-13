#include "cbase/c_allocator.h"
#include "cbase/c_context.h"
#include "cbase/c_memory.h"
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
            struct JsonState
            {
                JsonLexerState m_Lexer;
                char*          m_ErrorMessage;
                JsonAllocator* m_Allocator;
                int            m_NumberOfObjects;
                int            m_NumberOfNumbers;
                int            m_NumberOfStrings;
                int            m_NumberOfArrays;
                int            m_NumberOfBooleans;
                JsonValue*     m_NullValue;
            };

            static void JsonStateInit(JsonState* state, JsonAllocator* alloc, char const* buffer, char const* end)
            {
                JsonLexerStateInit(&state->m_Lexer, buffer, end, alloc);
                state->m_ErrorMessage      = nullptr;
                state->m_Allocator         = alloc;
                state->m_NumberOfObjects   = 0;
                state->m_NumberOfNumbers   = 0;
                state->m_NumberOfStrings   = 0;
                state->m_NumberOfArrays    = 0;
                state->m_NumberOfBooleans  = 2;
                state->m_NullValue         = alloc->Allocate<JsonValue>();
                state->m_NullValue->m_Type = JsonValue::kNull;
            }

            static JsonValue* JsonError(JsonState* state, const char* error)
            {
                state->m_ErrorMessage = state->m_Allocator->AllocateArray<char>(1024);
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

                bool seen_value = false;
                bool seen_comma = false;

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

                            JsonNamedValue* named_value = alloc->Allocate<JsonNamedValue>();
                            named_value->m_Name         = l.m_Str;
                            named_value->m_Value        = value;

                            JsonLinkedNamedValue* linked_value = alloc->Allocate<JsonLinkedNamedValue>();
                            linked_value->m_Value              = named_value;
                            linked_value->m_Next               = result->m_Value.m_Object.m_LinkedList;

                            result->m_Value.m_Object.m_LinkedList = linked_value;
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
                        sv->m_Value.m_String.m_String = l.m_Str;
                        sv->m_Value.m_String.m_End    = l.m_Str + l.m_Len;
                        result                        = sv;
                        JsonLexerSkip(&json_state->m_Lexer);
                        break;
                    }

                    case kJsonLexNumber:
                    {
                        json_state->m_NumberOfNumbers += 1;
                        JsonValue* nv                 = json_state->m_Allocator->Allocate<JsonValue>();
                        nv->m_Type                    = JsonValue::kNumber;
                        nv->m_Value.m_Number.m_String = l.m_Str;
                        nv->m_Value.m_Number.m_End    = l.m_Str + l.m_Len;
                        result                        = nv;
                        JsonLexerSkip(&json_state->m_Lexer);
                    }
                    break;

                    case kJsonLexBoolean:
                    {
                        JsonValue* nv                 = json_state->m_Allocator->Allocate<JsonValue>();
                        nv->m_Type                    = JsonValue::kBoolean;
                        nv->m_Value.m_Number.m_String = l.m_Str;
                        nv->m_Value.m_Number.m_End    = l.m_Str + l.m_Len;
                        result                        = nv;
                        JsonLexerSkip(&json_state->m_Lexer);
                    }
                    break;

                    case kJsonLexNull:
                    {
                        result = json_state->m_NullValue;
                        JsonLexerSkip(&json_state->m_Lexer);
                    }
                    break;

                    default: result = JsonError(json_state, "invalid document"); break;
                }

                return result;
            }

            const JsonValue* Scan(char const* str, char const* end, JsonAllocator* allocator, char const*& error_message)
            {
                JsonState json_state;
                JsonStateInit(&json_state, allocator, str, end);

                const JsonValue* root = JsonParseValue(&json_state);
                if (root && !JsonLexerExpect(&json_state.m_Lexer, kJsonLexEof))
                {
                    root = JsonError(&json_state, "data after document");
                }

                error_message = nullptr;
                if (!root)
                {
                    s32 const len    = ascii::strlen(json_state.m_ErrorMessage);
                    char*     errmsg = allocator->AllocateArray<char>(len + 1);
                    nmem::memcpy(errmsg, json_state.m_ErrorMessage, len);
                    errmsg[len]   = '\0';
                    error_message = errmsg;
                }

                return root;
            }
        } // namespace nscanner

    } // namespace njson
} // namespace ncore

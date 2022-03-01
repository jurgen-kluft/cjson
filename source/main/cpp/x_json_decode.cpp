#include "xbase/x_allocator.h"
#include "xbase/x_context.h"
#include "xbase/x_printf.h"
#include "xbase/x_runes.h"
#include "xjson/x_json.h"
#include "xjson/x_json_utils.h"
#include "xjson/x_json_allocator.h"
#include "xjson/x_json_decode.h"

namespace xcore
{
    namespace json
    {
        static void           JsonAllocBool(JsonAllocator* alloc, s32 n, void*& ptr) { ptr = alloc->AllocateArray<bool>(n); }
        static JsonMemberType sJsonTypeBool       = {JsonMemberType::TypeBool, JsonAllocBool, nullptr};
        static JsonMemberType sJsonTypeBoolPtr    = {JsonMemberType::TypeBool | JsonMemberType::TypePointer, JsonAllocBool, nullptr};
        static JsonMemberType sJsonTypeBoolVector = {JsonMemberType::TypeBool | JsonMemberType::TypeVector, JsonAllocBool, nullptr};
        static JsonMemberType sJsonTypeBoolCArray = {JsonMemberType::TypeBool | JsonMemberType::TypeCarray, JsonAllocBool, nullptr};
        JsonMemberType const* JsonTypeBool        = &sJsonTypeBool;
        JsonMemberType const* JsonTypeBoolPtr     = &sJsonTypeBoolPtr;
        JsonMemberType const* JsonTypeBoolVector  = &sJsonTypeBoolVector;
        JsonMemberType const* JsonTypeBoolCArray  = &sJsonTypeBoolCArray;

        JsonMember JsonObjectGetMember(JsonObject& object, const char* name) { return JsonMember(); }

    } // namespace json

    namespace json_decoder
    {
        using namespace json;

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
            JsonLexemeType m_Type;
            union
            {
                bool       m_Boolean;
                JsonNumber m_Number;
                char*      m_String;
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

        struct JsonError
        {
            const char* m_ErrorMessage;
        };

        static JsonError* MakeJsonError(JsonState* state, const char* error)
        {
            state->m_ErrorMessage = state->m_Allocator->AllocateArray<char>(1024);
            runes_t  errmsg(state->m_ErrorMessage, state->m_ErrorMessage + 1024 - 1);
            crunes_t fmt("line %d: %s");
            xcore::sprintf(errmsg, fmt, va_t(state->m_Lexer.m_LineNumber), va_t(error));
            return nullptr;
        }

        static JsonError* JsonDecodeValue(JsonState* json_state, JsonObject& object, JsonMember& member);

        static JsonError* JsonDecodeObject(JsonState* json_state, JsonObject& object)
        {
            JsonLexerState* lexer = &json_state->m_Lexer;

            if (!JsonLexerExpect(lexer, kJsonLexBeginObject))
                return MakeJsonError(json_state, "expected '{'");

            JsonAllocatorScope scratch_scope(json_state->m_Scratch);

            bool seen_value = false;
            bool seen_comma = false;

            bool done = false;
            while (!done)
            {
                JsonLexeme l = JsonLexerNext(lexer);

                switch (l.m_Type)
                {
                    case kJsonLexEndObject: done = true; break;

                    case kJsonLexString:
                    {
                        if (seen_value && !seen_comma)
                            return MakeJsonError(json_state, "expected ','");

                        if (!JsonLexerExpect(lexer, kJsonLexNameSeparator))
                            return MakeJsonError(json_state, "expected ':'");

                        JsonMember member = JsonObjectGetMember(object, l.m_String);
                        JsonError* err    = JsonDecodeValue(json_state, object, member);
                        if (err != nullptr)
                            return err;

                        seen_value = true;
                        seen_comma = false;
                    }
                    break;

                    case kJsonLexValueSeparator:
                    {
                        if (!seen_value)
                            return MakeJsonError(json_state, "expected key name");

                        if (seen_comma)
                            return MakeJsonError(json_state, "duplicate comma");

                        seen_value = false;
                        seen_comma = true;
                        break;
                    }

                    default: return MakeJsonError(json_state, "expected object to continue");
                }
            }

            return nullptr;
        }

        static JsonError* JsonDecodeArray(JsonState* json_state, JsonObject& object, JsonMember& member)
        {
            JsonLexerState* lexer = &json_state->m_Lexer;

            if (!JsonLexerExpect(lexer, kJsonLexBeginArray))
                return MakeJsonError(json_state, "expected '['");

            JsonAllocatorScope scratch_scope(json_state->m_Scratch);

            struct ListElem
            {
                JsonMember m_Member;
                ListElem*  m_Next;
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

                void Add(const JsonMember& member)
                {
                    if (1 == ++m_Count)
                    {
                        m_Head = m_Tail       = m_Scratch->Allocate<ListElem>();
                        m_Head->m_Member      = member;
                        m_Head->m_Next        = nullptr;
                    }
                    else
                    {
                        ListElem* tail        = m_Tail;
                        m_Tail                = m_Scratch->Allocate<ListElem>();
                        m_Tail->m_Member      = member;
                        m_Tail->m_Next        = nullptr;
                        tail->m_Next          = m_Tail;
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
                        return MakeJsonError(json_state, "expected ','");
                    }

                    JsonLexerSkip(lexer);
                }

                // The size of a member cannot be larger than a pointer.
                // f64, f32, u8/s8, u16/s16, u32/s32, u64/s64 as well as pointers to those types
                void* member_value = nullptr;

                void*      member_value_ptr = &member_value;
                JsonError* err              = JsonDecodeValue(json_state, object, member);
                if (err != nullptr)
                    return err;

                value_list.Add(member);
            }

            JsonAllocator* alloc = json_state->m_Allocator;

            s32 count = value_list.m_Count;

            // Allocate the correct type of array and populate it
            // then set the member on the object to the array

            return nullptr;
        }

        bool MemberIsObject(JsonMemberDescr const* m) { return false; }
        bool MemberIsArray(JsonMemberDescr const* m) { return false; }
        bool MemberIsString(JsonMemberDescr const* m) { return false; }
        bool MemberIsNumber(JsonMemberDescr const* m) { return false; }

        bool JsonObjectSetMemberNumber(JsonObject& object, JsonMember& member, JsonNumber const& number) { return false; }

        JsonError* JsonDecodeValue(JsonState* json_state, JsonObject& object, JsonMember& member)
        {
            JsonError* err = nullptr;
            JsonLexeme l   = JsonLexerPeek(&json_state->m_Lexer);
            switch (l.m_Type)
            {
                case kJsonLexBeginObject:
                {
                    if (!MemberIsObject(member.m_member_descr))
                        return MakeJsonError(json_state, "encountered json object but class member is not the same type");

                    JsonObject member_object;
                    member_object.m_object_descr = member.m_member_descr->m_type->m_object;

                    if (member.m_instance == nullptr)
                    {
                        if (member.m_member_descr->m_type->is_pointer())
                            member.m_member_descr->m_type->m_alloc(json_state->m_Allocator, 1, member_object.m_instance);
                        else
                            member_object.m_instance = JsonObjectGetMemberPtr(object, member);
                    }
                    else
                    {
                        // The member value is part of something else, like an array element.
                        // The caller has provided a pointer to the member value.
                    }

                    err = JsonDecodeObject(json_state, member_object);
                }
                break;
                case kJsonLexBeginArray:
                    if (!MemberIsArray(member.m_member_descr))
                        return MakeJsonError(json_state, "encountered json array but class member is not the same type");

                    err = JsonDecodeArray(json_state, object, member);
                    break;

                case kJsonLexString:
                {
                    if (!MemberIsString(member.m_member_descr))
                        return MakeJsonError(json_state, "encountered json string but class member is not the same type");

                    json_state->m_NumberOfStrings += 1;

                    if (member.m_instance == nullptr)
                    {
                        member.m_instance = JsonObjectGetMemberPtr(object, member);
                    }
                    else
                    {
                        // The member value is part of something else, like an array element
                        // The caller has provided a pointer to the member value.
                    }

                    const char** string_data = (const char**)member.m_instance;
                    *string_data             = l.m_String;

                    JsonLexerSkip(&json_state->m_Lexer);
                    break;
                }

                case kJsonLexNumber:
                {
                    if (!MemberIsNumber(member.m_member_descr))
                        return MakeJsonError(json_state, "encountered json number but class member is not the same type");

                    json_state->m_NumberOfNumbers += 1;
                    JsonNumber const& number = l.m_Number;

                    if (member.m_instance == nullptr)
                    {
                        if (member.m_member_descr->m_type->is_pointer())
                        {
                            void** member_value_ptr = (void**)JsonObjectGetMemberPtr(object, member);
                            void*  value_ptr;
                            member.m_member_descr->m_type->m_alloc(json_state->m_Allocator, 1, value_ptr);
                            *member_value_ptr = value_ptr;
                            member.m_instance = value_ptr;
                        }
                        else
                        {
                            JsonObjectSetMemberNumber(object, member, number);
                            // Write the number to the correct member type
                        }
                    }
                    else
                    {
                        // The member value is part of something else, like an array element
                        // The caller has provided a pointer to the member value.
                    }

                    JsonLexerSkip(&json_state->m_Lexer);
                    break;
                }

                case kJsonLexBoolean:
                    if (!MemberIsNumber(member.m_member_descr))
                        return MakeJsonError(json_state, "encountered json boolean but class member is not the same type");

                    // Set the boolean value on the object member

                    JsonLexerSkip(&json_state->m_Lexer);
                    break;

                case kJsonLexNull: JsonLexerSkip(&json_state->m_Lexer); break;

                default: return MakeJsonError(json_state, "invalid document");
            }

            return nullptr;
        }

        bool JsonDecode(char const* str, char const* end, JsonObject& json_root, JsonAllocator* allocator, JsonAllocator* scratch, char const*& error_message)
        {
            JsonState* json_state = scratch->Allocate<JsonState>();
            JsonStateInit(json_state, allocator, scratch, str, end);

            JsonError* error = JsonDecodeObject(json_state, json_root);
            if (error == nullptr)
            {
                if (!JsonLexerExpect(&json_state->m_Lexer, kJsonLexEof))
                {
                    error = MakeJsonError(json_state, "data after document");
                }
                else
                {
                    return true;
                }
            }

            // Mainly to reduce the duplication of keys (strings) but in some way also string values.
            // We could benefit from an additional 2 allocators that are used for:
            // - [Hash 32-bit, String Offset 32-bit] pairs (sorted)
            // - Strings (UTF-8 strings)
            // The user could even pre-warm these with known strings and even reuse them between different JSON documents.

            error_message    = nullptr;
            s32 const len    = ascii::strlen(json_state->m_ErrorMessage);
            char*     errmsg = scratch->AllocateArray<char>(len + 1);
            x_memcopy(errmsg, json_state->m_ErrorMessage, len);
            errmsg[len]   = '\0';
            error_message = errmsg;
            return false;
        }

    } // namespace json_decoder
} // namespace xcore
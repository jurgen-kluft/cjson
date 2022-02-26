#include "xbase/x_allocator.h"
#include "xbase/x_context.h"
#include "xbase/x_runes.h"
#include "xjson/x_json.h"

#include <stddef.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _MSC_VER
#define snprintf _snprintf
#endif

namespace xcore
{
    namespace json
    {
        typedef s32 ThreadId;

        struct MemAllocLinear
        {
            enum
            {
                kMaxAlignment = 64
            };

            char*       m_BasePointer; // allocated pointer
            char*       m_Pointer;     // aligned pointer
            int         m_Size;
            int         m_Offset;
            const char* m_DebugName;
            ThreadId    m_OwnerThread;

            void  Init(int max_size, const char* debug_name);
            void  Destroy();
            void  SetOwner(ThreadId thread_id);
            void* Allocate(int size, int align);
            void  Reset();

            template <typename T> T* Allocate() { return static_cast<T*>(Allocate(sizeof(T), ALIGNOF(T))); }
            template <typename T> T* AllocateArray(int count) { return static_cast<T*>(Allocate(sizeof(T) * count, ALIGNOF(T))); }
        };

        class MemAllocLinearScope
        {
            MemAllocLinear* m_Allocator;
            int             m_Offset;

        public:
            explicit MemAllocLinearScope(MemAllocLinear* a)
                : m_Allocator(a)
                , m_Offset(a->m_Offset)
            {
            }

            ~MemAllocLinearScope() { m_Allocator->m_Offset = m_Offset; }

        private:
            MemAllocLinearScope(const MemAllocLinearScope&);
            MemAllocLinearScope& operator=(const MemAllocLinearScope&);
        };

        inline char* StrDupN(MemAllocLinear* allocator, const char* str, int len)
        {
            int   sz     = len + 1;
            char* buffer = static_cast<char*>(allocator->Allocate(sz, 1));
            memcpy(buffer, str, sz - 1);
            buffer[sz - 1] = '\0';
            return buffer;
        }

        inline char* StrDup(MemAllocLinear* allocator, const char* str) { return StrDupN(allocator, str, strlen(str)); }

#define CHECK_THREAD_OWNERSHIP(alloc) ASSERT(context_t::thread_index() == alloc->m_OwnerThread)

        void MemAllocLinear::Init(int max_size, const char* debug_name)
        {
            int alloc_size      = max_size + MemAllocLinear::kMaxAlignment - 1;
            this->m_BasePointer = static_cast<char*>(context_t::system_alloc()->allocate(alloc_size));
            this->m_Pointer     = nullptr;
            this->m_Size        = max_size;
            this->m_Offset      = 0;
            this->m_DebugName   = debug_name;

            uintptr_t aligned_base = uintptr_t(this->m_BasePointer + MemAllocLinear::kMaxAlignment - 1) & ~(MemAllocLinear::kMaxAlignment - 1);
            uintptr_t delta        = aligned_base - uintptr_t(this->m_BasePointer);

            this->m_Pointer = this->m_BasePointer + delta;
            this->m_Size -= (delta);

            this->m_OwnerThread = context_t::thread_index();
        }

        void MemAllocLinear::Destroy()
        {
            context_t::system_alloc()->deallocate(this->m_BasePointer);
            this->m_BasePointer = nullptr;
        }

        void  MemAllocLinear::SetOwner(ThreadId thread_id) { this->m_OwnerThread = thread_id; }
        void* MemAllocLinear::Allocate(int size, int align)
        {
            CHECK_THREAD_OWNERSHIP(this);

            // Alignment must be power of two
            ASSERT(0 == (align & (align - 1)));

            // Alignment must be non-zero
            ASSERT(align > 0);

            // Compute aligned offset.
            int offset = (this->m_Offset + align - 1) & ~(align - 1);

            ASSERT(0 == (offset & (align - 1)));

            // See if we have space.
            if (offset + size <= this->m_Size)
            {
                char* ptr      = this->m_Pointer + offset;
                this->m_Offset = offset + size;
                return ptr;
            }
            else
            {
                ASSERTS(false, "Out of memory in linear allocator (json parser)");
                return nullptr;
            }
        }

        void MemAllocLinear::Reset()
        {
            CHECK_THREAD_OWNERSHIP(this);
            this->m_Offset = 0;
        }

        static JsonBooleanValue s_TrueValue;
        static JsonBooleanValue s_FalseValue;
        static const JsonValue  s_NullValue = {JsonValue::kNull};

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

        static const JsonLexeme s_ValueSeparatorLexeme = {kJsonLexValueSeparator, {0}};
        static const JsonLexeme s_NameSeparatorLexeme  = {kJsonLexNameSeparator, {0}};
        static const JsonLexeme s_BeginObjectLexeme    = {kJsonLexBeginObject, {0}};
        static const JsonLexeme s_EndObjectLexeme      = {kJsonLexEndObject, {0}};
        static const JsonLexeme s_BeginArrayLexeme     = {kJsonLexBeginArray, {0}};
        static const JsonLexeme s_EndArrayLexeme       = {kJsonLexEndArray, {0}};
        static const JsonLexeme s_NullLexeme           = {kJsonLexNull, {0}};
        static const JsonLexeme s_EofLexeme            = {kJsonLexEof, {0}};
        static const JsonLexeme s_ErrorLexeme          = {kJsonLexError, {0}};
        static const JsonLexeme s_TrueLexeme           = {kJsonLexBoolean, {true}};
        static const JsonLexeme s_FalseLexeme          = {kJsonLexBoolean, {false}};

        struct JsonLexerState
        {
            char*      m_Cursor;
            int        m_LineNumber;
            JsonLexeme m_Lexeme;
            char       m_Error[1024];
        };

        static void JsonLexerStateInit(JsonLexerState* self, char* buffer)
        {
            self->m_Cursor        = buffer;
            self->m_LineNumber    = 1;
            self->m_Lexeme.m_Type = kJsonLexInvalid;
            self->m_Error[0]      = '\0';
        }

        static char* SkipWhitespace(JsonLexerState* state)
        {
            char* ptr = state->m_Cursor;

            while (char ch = *ptr)
            {
                if ('\n' == ch)
                {
                    ++state->m_LineNumber;
                }

                if (isspace(ch))
                    ++ptr;
                else
                    break;
            }

            state->m_Cursor = ptr;
            return ptr;
        }

        static JsonLexeme JsonLexerError(JsonLexerState* state, const char* error)
        {
            snprintf(state->m_Error, sizeof state->m_Error, "%d: %s", state->m_LineNumber, error);
            return s_ErrorLexeme;
        }

        static JsonLexeme GetNumberLexeme(JsonLexerState* state)
        {
            char* start = state->m_Cursor;
            char* end   = nullptr;

            JsonLexeme result;

            result.m_Type   = kJsonLexNumber;
            result.m_Number = strtod(start, &end);
            state->m_Cursor = end;
            return start != end ? result : JsonLexerError(state, "bad number");
        }

        static JsonLexeme GetStringLexeme(JsonLexerState* state)
        {
            JsonLexeme result;

            char* rptr = state->m_Cursor;
            char* wptr = rptr;
            ++rptr; // skip quote

            result.m_Type   = kJsonLexString;
            result.m_String = wptr;

            for (;;)
            {
                char ch = *rptr++;
                if (0 == ch)
                    return JsonLexerError(state, "end of file inside string");

                if ('"' == ch)
                {
                    *wptr = '\0';
                    break;
                }
                else if ('\\' == ch)
                {
                    char next = *rptr++;
                    switch (next)
                    {
                        case '\\': *wptr++ = '\\'; break;
                        case '"': *wptr++ = '"'; break;
                        case '/': *wptr++ = '/'; break;
                        case 'b': *wptr++ = '\b'; break;
                        case 'f': *wptr++ = '\f'; break;
                        case 'n': *wptr++ = '\n'; break;
                        case 'r': *wptr++ = '\r'; break;
                        case 't': *wptr++ = '\t'; break;
                        case 'u':
                        {
                            uint32_t hex_code = 0;
                            for (int i = 0; i < 4; ++i)
                            {
                                char code = *rptr++;
                                if (0 == code)
                                {
                                    return JsonLexerError(state, "end of file inside escape");
                                }
                                else if (is_hexa(code))
                                {
                                    int lc = to_lower(code);
                                    hex_code <<= 4;
                                    if (lc >= 'a' && lc <= 'f')
                                        hex_code |= lc - 'a' + 10;
                                    else
                                        hex_code |= lc - '0';
                                }
                                else
                                {
                                    return JsonLexerError(state, "expected hex number in \\u escape");
                                }
                            }

                            if (hex_code > 127)
                                return JsonLexerError(state, "we currently only support ASCII");

                            *wptr++ = (char)hex_code;
                            break;
                        }

                        default: return JsonLexerError(state, "unsupported escape code");
                    }
                }
                else
                {
                    *wptr++ = ch;
                }
            }

            state->m_Cursor = rptr;
            return result;
        }

        static JsonLexeme GetLiteralLexeme(JsonLexerState* state)
        {
            char* rptr = state->m_Cursor;
            char* eptr = rptr;
            while (isalnum(*eptr))
            {
                eptr++;
            }

            int kwlen = (eptr - rptr);

            if (4 == kwlen)
            {
                if (0 == strncmp("true", rptr, 4))
                {
                    state->m_Cursor = eptr;
                    return s_TrueLexeme;
                }

                else if (0 == strncmp("null", rptr, 4))
                {
                    state->m_Cursor = eptr;
                    return s_NullLexeme;
                }
            }

            else if (5 == kwlen)
            {
                if (0 == strncmp("false", rptr, 5))
                {
                    state->m_Cursor = eptr;
                    return s_FalseLexeme;
                }
            }

            return JsonLexerError(state, "invalid literal, expected one of false, true or null");
        }

        static JsonLexeme JsonLexerFetchNext(JsonLexerState* state)
        {
            char* p  = SkipWhitespace(state);
            char  ch = *p;

            switch (ch)
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

                case '{': state->m_Cursor = p + 1; return s_BeginObjectLexeme;

                case '}': state->m_Cursor = p + 1; return s_EndObjectLexeme;

                case '[': state->m_Cursor = p + 1; return s_BeginArrayLexeme;

                case ']': state->m_Cursor = p + 1; return s_EndArrayLexeme;

                case ',': state->m_Cursor = p + 1; return s_ValueSeparatorLexeme;

                case ':': state->m_Cursor = p + 1; return s_NameSeparatorLexeme;

                case '\0': return s_EofLexeme;

                default: return GetLiteralLexeme(state);
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
            JsonLexerState  m_Lexer;
            char            m_ErrorMessage[1024];
            MemAllocLinear* m_Allocator;
            MemAllocLinear* m_Scratch;
        };

        static void JsonStateInit(JsonState* state, MemAllocLinear* alloc, MemAllocLinear* scratch, char* buffer)
        {
            JsonLexerStateInit(&state->m_Lexer, buffer);
            state->m_ErrorMessage[0] = '\0';
            state->m_Allocator       = alloc;
            state->m_Scratch         = scratch;
        }

        static JsonValue* JsonError(JsonState* state, const char* error)
        {
            snprintf(state->m_ErrorMessage, sizeof state->m_ErrorMessage, "line %d: %s", state->m_Lexer.m_LineNumber, error);
            return nullptr;
        }

        static const JsonValue* JsonParseValue(JsonState* json_state);

        static const JsonValue* JsonParseObject(JsonState* json_state)
        {
            JsonLexerState* lexer = &json_state->m_Lexer;

            if (!JsonLexerExpect(lexer, kJsonLexBeginObject))
                return JsonError(json_state, "expected '{'");

            MemAllocLinearScope scratch_scope(json_state->m_Scratch);

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
                MemAllocLinear* m_Scratch;
                KvPair*         m_Head;
                KvPair*         m_Tail;
                int             m_Count;

                void Init(MemAllocLinear* scratch)
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

            MemAllocLinear* alloc = json_state->m_Allocator;

            int               count  = kv_pairs.m_Count;
            const char**      names  = alloc->AllocateArray<const char*>(kv_pairs.m_Count);
            const JsonValue** values = alloc->AllocateArray<const JsonValue*>(kv_pairs.m_Count);

            int index = 0;
            for (KvPair* p = kv_pairs.m_Head; p; p = p->m_Next, ++index)
            {
                names[index]  = p->m_Key;
                values[index] = p->m_Value;
            }

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

            MemAllocLinearScope scratch_scope(json_state->m_Scratch);

            struct ListElem
            {
                const JsonValue* m_Value;
                ListElem*        m_Next;
            };

            struct ValueList
            {
                MemAllocLinear* m_Scratch;
                ListElem*       m_Head;
                ListElem*       m_Tail;
                int             m_Count;

                void Init(MemAllocLinear* scratch)
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

            MemAllocLinear* alloc = json_state->m_Allocator;

            int               count  = value_list.m_Count;
            const JsonValue** values = alloc->AllocateArray<const JsonValue*>(count);

            int index = 0;
            for (ListElem* p = value_list.m_Head; p; p = p->m_Next, ++index)
            {
                values[index] = p->m_Value;
            }

            JsonArrayValue* result = alloc->Allocate<JsonArrayValue>();
            result->m_Type         = JsonValue::kArray;
            result->m_Count        = count;
            result->m_Values       = values;

            return result;
        }

        static const JsonValue* JsonParseValue(JsonState* json_state)
        {
            MemAllocLinear* alloc = json_state->m_Allocator;
            JsonLexerState* lexer = &json_state->m_Lexer;
            JsonLexeme      l     = JsonLexerPeek(lexer);

            const JsonValue* result = nullptr;

            switch (l.m_Type)
            {
                case kJsonLexBeginObject: result = JsonParseObject(json_state); break;

                case kJsonLexBeginArray: result = JsonParseArray(json_state); break;

                case kJsonLexString:
                {
                    JsonStringValue* sv = alloc->Allocate<JsonStringValue>();
                    sv->m_Type          = JsonValue::kString;
                    sv->m_String        = l.m_String;
                    result              = sv;
                    JsonLexerSkip(lexer);
                    break;
                }

                case kJsonLexNumber:
                {
                    JsonNumberValue* nv = alloc->Allocate<JsonNumberValue>();
                    nv->m_Type          = JsonValue::kNumber;
                    nv->m_Number        = l.m_Number;
                    result              = nv;
                    JsonLexerSkip(lexer);
                    break;
                }

                case kJsonLexBoolean:
                    result = l.m_Boolean ? &s_TrueValue : &s_FalseValue;
                    JsonLexerSkip(lexer);
                    break;

                case kJsonLexNull:
                    result = &s_NullValue;
                    JsonLexerSkip(lexer);
                    break;

                default: result = JsonError(json_state, "invalid document"); break;
            }

            return result;
        }

        const JsonValue* JsonParse(char* buffer, MemAllocLinear* allocator, MemAllocLinear* scratch, char (&error_message)[1024])
        {
            // Setup statics. Harmless to do multiple times.
            s_TrueValue.m_Type     = JsonValue::kBoolean;
            s_TrueValue.m_Boolean  = true;
            s_FalseValue.m_Type    = JsonValue::kBoolean;
            s_FalseValue.m_Boolean = false;

            JsonState* json_state = scratch->Allocate<JsonState>();
            JsonStateInit(json_state, allocator, scratch, buffer);

            const JsonValue* root = JsonParseValue(json_state);

            if (root && !JsonLexerExpect(&json_state->m_Lexer, kJsonLexEof))
            {
                root = JsonError(json_state, "data after document");
            }

            if (root)
            {
                error_message[0] = '\0';
            }
            else
            {
                strncpy(error_message, json_state->m_ErrorMessage, sizeof error_message);
                error_message[sizeof(error_message) - 1] = '\0';
            }

            return root;
        }

    } // namespace json
} // namespace xcore
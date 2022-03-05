#include "xbase/x_allocator.h"
#include "xbase/x_context.h"
#include "xbase/x_printf.h"
#include "xbase/x_runes.h"
#include "xjson/x_json.h"
#include "xjson/x_json_utils.h"
#include "xjson/x_json_allocator.h"
#include "xjson/x_json_decode.h"
#include "xjson/x_json_lexer.h"

namespace xcore
{
    namespace json
    {
        static void          JsonAllocBool(JsonAllocator* alloc, s32 n, void*& ptr) { ptr = alloc->AllocateArray<bool>(n); }
        static JsonTypeFuncs sJsonFuncsBool = {JsonAllocBool, nullptr};
        JsonTypeFuncs const* JsonFuncsBool  = &sJsonFuncsBool;

        static void          JsonAllocInt8(JsonAllocator* alloc, s32 n, void*& ptr) { ptr = alloc->AllocateArray<s8>(n); }
        static JsonTypeFuncs sJsonFuncsInt8 = {JsonAllocInt8, nullptr};
        JsonTypeFuncs const* JsonFuncsInt8  = &sJsonFuncsInt8;

        static void          JsonAllocInt16(JsonAllocator* alloc, s32 n, void*& ptr) { ptr = alloc->AllocateArray<s16>(n); }
        static JsonTypeFuncs sJsonFuncsInt16 = {JsonAllocInt16, nullptr};
        JsonTypeFuncs const* JsonFuncsInt16  = &sJsonFuncsInt16;

        static void          JsonAllocInt32(JsonAllocator* alloc, s32 n, void*& ptr) { ptr = alloc->AllocateArray<s32>(n); }
        static JsonTypeFuncs sJsonFuncsInt32 = {JsonAllocInt32, nullptr};
        JsonTypeFuncs const* JsonFuncsInt32  = &sJsonFuncsInt32;

        static void          JsonAllocInt64(JsonAllocator* alloc, s32 n, void*& ptr) { ptr = alloc->AllocateArray<s64>(n); }
        static JsonTypeFuncs sJsonFuncsInt64 = {JsonAllocInt64, nullptr};
        JsonTypeFuncs const* JsonFuncsInt64  = &sJsonFuncsInt64;

        static void          JsonAllocUInt8(JsonAllocator* alloc, s32 n, void*& ptr) { ptr = alloc->AllocateArray<u8>(n); }
        static JsonTypeFuncs sJsonFuncsUInt8 = {JsonAllocUInt8, nullptr};
        JsonTypeFuncs const* JsonFuncsUInt8  = &sJsonFuncsUInt8;

        static void          JsonAllocUInt16(JsonAllocator* alloc, s32 n, void*& ptr) { ptr = alloc->AllocateArray<u16>(n); }
        static JsonTypeFuncs sJsonFuncsUInt16 = {JsonAllocUInt16, nullptr};
        JsonTypeFuncs const* JsonFuncsUInt16  = &sJsonFuncsUInt16;

        static void          JsonAllocUInt32(JsonAllocator* alloc, s32 n, void*& ptr) { ptr = alloc->AllocateArray<u32>(n); }
        static JsonTypeFuncs sJsonFuncsUInt32 = {JsonAllocUInt32, nullptr};
        JsonTypeFuncs const* JsonFuncsUInt32  = &sJsonFuncsUInt32;

        static void          JsonAllocUInt64(JsonAllocator* alloc, s32 n, void*& ptr) { ptr = alloc->AllocateArray<u64>(n); }
        static JsonTypeFuncs sJsonFuncsUInt64 = {JsonAllocUInt64, nullptr};
        JsonTypeFuncs const* JsonFuncsUInt64  = &sJsonFuncsUInt64;

        static void          JsonAllocFloat32(JsonAllocator* alloc, s32 n, void*& ptr) { ptr = alloc->AllocateArray<f32>(n); }
        static JsonTypeFuncs sJsonFuncsFloat32 = {JsonAllocFloat32, nullptr};
        JsonTypeFuncs const* JsonFuncsFloat32  = &sJsonFuncsFloat32;

        static void          JsonAllocFloat64(JsonAllocator* alloc, s32 n, void*& ptr) { ptr = alloc->AllocateArray<f64>(n); }
        static JsonTypeFuncs sJsonFuncsFloat64 = {JsonAllocFloat64, nullptr};
        JsonTypeFuncs const* JsonFuncsFloat64  = &sJsonFuncsFloat64;

        static void          JsonAllocString(JsonAllocator* alloc, s32 n, void*& ptr) {}
        static JsonTypeFuncs sJsonFuncsString = {JsonAllocString, nullptr};
        JsonTypeFuncs const* JsonFuncsString  = &sJsonFuncsString;

        static bool          sDefaultBool       = false;
        static JsonTypeDescr sJsonTypeDescrBool = {"bool", &sDefaultBool, sizeof(bool), ALIGNOF(bool), 0, nullptr};

        static s8            sDefaultInt8       = 0;
        static JsonTypeDescr sJsonTypeDescrInt8 = {"s8", &sDefaultInt8, sizeof(s8), ALIGNOF(s8), 0, nullptr};

        static s16           sDefaultInt16       = 0;
        static JsonTypeDescr sJsonTypeDescrInt16 = {"s16", &sDefaultInt16, sizeof(s16), ALIGNOF(s16), 0, nullptr};

        static s32           sDefaultInt32       = 0;
        static JsonTypeDescr sJsonTypeDescrInt32 = {"s32", &sDefaultInt32, sizeof(s32), ALIGNOF(s32), 0, nullptr};

        static s64           sDefaultInt64       = 0;
        static JsonTypeDescr sJsonTypeDescrInt64 = {"s64", &sDefaultInt64, sizeof(s64), ALIGNOF(s64), 0, nullptr};

        static u8            sDefaultUInt8       = 0;
        static JsonTypeDescr sJsonTypeDescrUInt8 = {"u8", &sDefaultUInt8, sizeof(u8), ALIGNOF(u8), 0, nullptr};

        static u16           sDefaultUInt16       = 0;
        static JsonTypeDescr sJsonTypeDescrUInt16 = {"u16", &sDefaultUInt16, sizeof(u16), ALIGNOF(u16), 0, nullptr};

        static u32           sDefaultUInt32       = 0;
        static JsonTypeDescr sJsonTypeDescrUInt32 = {"u32", &sDefaultUInt32, sizeof(u32), ALIGNOF(u32), 0, nullptr};

        static u64           sDefaultUInt64       = 0;
        static JsonTypeDescr sJsonTypeDescrUInt64 = {"u64", &sDefaultUInt64, sizeof(u64), ALIGNOF(u64), 0, nullptr};

        static f32           sDefaultFloat32       = 0.0f;
        static JsonTypeDescr sJsonTypeDescrFloat32 = {"f32", &sDefaultFloat32, sizeof(f32), ALIGNOF(f32), 0, nullptr};

        static f64           sDefaultFloat64       = 0.0f;
        static JsonTypeDescr sJsonTypeDescrFloat64 = {"f64", &sDefaultFloat64, sizeof(f64), ALIGNOF(f64), 0, nullptr};

        static const char*   sDefaultString       = "";
        static JsonTypeDescr sJsonTypeDescrString = {"string", &sDefaultString, sizeof(const char*), ALIGNOF(const char*), 0, nullptr};

        JsonTypeDescr const* JsonTypeDescrBool    = &sJsonTypeDescrBool;
        JsonTypeDescr const* JsonTypeDescrInt8    = &sJsonTypeDescrInt8;
        JsonTypeDescr const* JsonTypeDescrInt16   = &sJsonTypeDescrInt16;
        JsonTypeDescr const* JsonTypeDescrInt32   = &sJsonTypeDescrInt32;
        JsonTypeDescr const* JsonTypeDescrInt64   = &sJsonTypeDescrInt64;
        JsonTypeDescr const* JsonTypeDescrUInt8   = &sJsonTypeDescrUInt8;
        JsonTypeDescr const* JsonTypeDescrUInt16  = &sJsonTypeDescrUInt16;
        JsonTypeDescr const* JsonTypeDescrUInt32  = &sJsonTypeDescrUInt32;
        JsonTypeDescr const* JsonTypeDescrUInt64  = &sJsonTypeDescrUInt64;
        JsonTypeDescr const* JsonTypeDescrFloat32 = &sJsonTypeDescrFloat32;
        JsonTypeDescr const* JsonTypeDescrFloat64 = &sJsonTypeDescrFloat64;
        JsonTypeDescr const* JsonTypeDescrString  = &sJsonTypeDescrString;

		void* JsonMember::get_member_ptr(JsonObject const& object)
		{
			uptr offset = (uptr)m_descr->m_member - (uptr)object.m_descr->m_default;
			return (void*)((uptr)object.m_instance + offset);
		}

        JsonObject JsonMember::get_object(JsonObject const& object, JsonAllocator* alloc)
        {
            JsonObject o;
            o.m_descr = m_descr->m_typedescr;
            if (m_data_ptr != nullptr)
            {
                // the instance is likely an element of an array, so the data ptr of the
                // member is already set
                o.m_instance = m_data_ptr;
            }
            else
            {
                if (is_pointer())
                {
                    // the instance of the object is not part of the instance of the main object
                    // e.g. as a struct member:
                    //                           key_t*  m_key;
                    //
                    // Once allocated, write the pointer of the object into the member
                    m_descr->m_funcs->m_alloc(alloc, 1, o.m_instance);
                    void** member_ptr = (void**)get_member_ptr(object);
                    *member_ptr       = o.m_instance;
                }
                else
                {
                    // the instance of the object is already part of the instance of the main object
                    // e.g. as a struct member:
                    //                           key_t  m_key;
                    //
                    o.m_instance = get_member_ptr(object);
                }
            }

            return o;
        }

        void JsonMember::set_string(JsonObject const& object, const char* str)
        {
            if (has_descr())
            {
                // Set the string pointer on the object member
                if (m_data_ptr == nullptr)
                {
                    m_data_ptr = (void*)get_member_ptr(object);
                }
                else
                {
                    // The member value is part of something else, like an array element
                    // The caller has provided a pointer to the member value.
                }

                *((const char**)m_data_ptr) = str;
            }
        }

        void JsonMember::set_number(JsonObject const& object, JsonAllocator* alloc, JsonNumber const& number)
        {
            if (has_descr())
            {
                // Set the number on the object member
                if (m_data_ptr == nullptr)
                {
                    if (is_pointer())
                    {
                        void** member_value_ptr = (void**)get_member_ptr(object);
                        void*  value_ptr;
                        m_descr->m_funcs->m_alloc(alloc, 1, value_ptr);
                        *member_value_ptr = value_ptr;
                        m_data_ptr        = value_ptr;
                    }
                    else
                    {
                        m_data_ptr = (void*)get_member_ptr(object);
                    }
                }
                else
                {
                    // The member value is part of something else, like an array element
                    // The caller has provided a pointer to the member value.
                }

                switch (m_descr->m_type & JsonType::TypeNumber)
                {
                    case JsonType::TypeInt8: *((s8*)m_data_ptr) = (s8)JsonNumberAsInt64(number); break;
                    case JsonType::TypeInt16: *((s16*)m_data_ptr) = (s16)JsonNumberAsInt64(number); break;
                    case JsonType::TypeInt32: *((s32*)m_data_ptr) = (s32)JsonNumberAsInt64(number); break;
                    case JsonType::TypeInt64: *((s64*)m_data_ptr) = JsonNumberAsInt64(number); break;
                    case JsonType::TypeUInt8: *((u8*)m_data_ptr) = (u8)JsonNumberAsUInt64(number); break;
                    case JsonType::TypeUInt16: *((u16*)m_data_ptr) = (u16)JsonNumberAsUInt64(number); break;
                    case JsonType::TypeUInt32: *((u32*)m_data_ptr) = (u32)JsonNumberAsUInt64(number); break;
                    case JsonType::TypeUInt64: *((u64*)m_data_ptr) = JsonNumberAsUInt64(number); break;
                    case JsonType::TypeF32: *((f32*)m_data_ptr) = (f32)JsonNumberAsFloat64(number); break;
                    case JsonType::TypeF64: *((f64*)m_data_ptr) = JsonNumberAsFloat64(number); break;
                    default: break;
                }
            }
        }

        void JsonMember::set_bool(JsonObject const& object, JsonAllocator* alloc, bool b)
        {
            if (has_descr())
            {
                // Set the boolean value on the object member
                if (m_data_ptr == nullptr)
                {
                    if (is_pointer())
                    {
                        void** member_value_ptr = (void**)get_member_ptr(object);
                        void*  value_ptr;
                        m_descr->m_funcs->m_alloc(alloc, 1, value_ptr);
                        *member_value_ptr = value_ptr;
                        m_data_ptr        = value_ptr;
                    }
                    else
                    {
                        m_data_ptr = (void*)get_member_ptr(object);
                    }
                }
                else
                {
                    // The member value is part of something else, like an array element
                    // The caller has provided a pointer to the member value.
                }

                *((bool*)m_data_ptr) = b;
            }
        }

        JsonMember JsonObject::get_member(const char* name) const
        {
            JsonMember member;
            if (m_descr != nullptr)
            {
                for (s32 i = 0; i < m_descr->m_member_count; ++i)
                {
                    const char* n1 = name;
                    const char* n2 = m_descr->m_members[i].m_name;
                    while (true)
                    {
                        if (*n1 != *n2)
                            break;

                        if (*n1 == 0)
                        {
                            member.m_descr = &m_descr->m_members[i];
                            return member;
                        }
                        n1++;
                        n2++;
                    }
                }
            }
            return member;
        }

    } // namespace json

    namespace json_decoder
    {
        using namespace json;

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

                        JsonMember member = object.get_member(l.m_String);
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
                union
                {
                    void* m_ElemData;
                    u64   m_ElemData64;
                };
                ListElem* m_Next;
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

                ListElem* NewListItem()
                {
                    ListElem* elem     = m_Scratch->Allocate<ListElem>();
                    elem->m_Next       = nullptr;
                    elem->m_ElemData64 = 0;
                    return elem;
                }

                void Add(ListElem* elem)
                {
                    m_Tail->m_Next = elem;
                    m_Tail         = elem;
                    m_Count += 1;
                }
            };

            JsonAllocator* scratch = json_state->m_Scratch;

            ValueList value_list;
            value_list.Init(scratch);

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

                JsonError* err = nullptr;
                if (member.has_descr())
                {
                    ListElem* elem = value_list.NewListItem();

                    JsonMember m = member;
                    if (m.is_pointer() || m.is_string() || m.is_number() || m.is_bool())
                    {
                        m.m_data_ptr = &elem->m_ElemData64;
                    }
                    else if (m.is_object())
                    {
                        void* obj;
                        m.m_descr->m_funcs->m_alloc(scratch, 1, obj);
                        elem->m_ElemData = obj;
                        m.m_data_ptr     = obj;
                    }
                    err = JsonDecodeValue(json_state, object, m);

                    value_list.Add(elem);
                }
                else
                {
                    err = JsonDecodeValue(json_state, object, member);
                }
                if (err != nullptr)
                    return err;
            }

            if (member.has_descr())
            {
                JsonAllocator* alloc = json_state->m_Allocator;

                s32   count = value_list.m_Count;
                void* array = nullptr;
                if (member.is_array_ptr())
                {
                    if (member.is_array_ptr_size8())
                    {
                        if (count > 127)
                            count = 127;

                        uptr const offset = (uptr)member.m_descr->m_size8 - (uptr)object.m_descr->m_default;
                        s8*        size8  = (s8*)((uptr)object.m_instance + offset);
                        *size8            = (s8)count;
                    }
                    else if (member.is_array_ptr_size16())
                    {
                        if (count > 32767)
                            count = 32767;
                        uptr const offset = (uptr)member.m_descr->m_size16 - (uptr)object.m_descr->m_default;
                        s16*       size16 = (s16*)((uptr)object.m_instance + offset);
                        *size16           = (s16)count;
                    }
                    else if (member.is_array_ptr_size32())
                    {
                        if (count > 2147483647)
                            count = 2147483647;
                        uptr const offset = (uptr)member.m_descr->m_size32 - (uptr)object.m_descr->m_default;
                        s32*       size32 = (s32*)((uptr)object.m_instance + offset);
                        *size32           = count;
                    }
                    member.m_descr->m_funcs->m_alloc(alloc, count, array);
                }
                else if (member.is_array())
                {
                    array = member.get_member_ptr(object);
                    count = count > member.m_descr->m_csize ? member.m_descr->m_csize : count;
                }
                else
                {
                    return MakeJsonError(json_state, "expected either a carray or vector");
                }

                ListElem* elem = value_list.m_Head.m_Next;

                if (!member.is_pointer())
                {
                    if (member.is_number())
                    {
                        if (member.is_bool())
                        {
                            bool* carray = (bool*)array;
                            for (s32 i = 0; i < count; ++i)
                            {
                                *carray++ = *((bool*)(&elem->m_ElemData64));
                                elem      = elem->m_Next;
                            }
                        }
                        else if (member.is_int8() || member.is_uint8())
                        {
                            u8* carray = (u8*)array;
                            for (s32 i = 0; i < count; ++i)
                            {
                                *carray++ = *((u8*)(&elem->m_ElemData64));
                                elem      = elem->m_Next;
                            }
                        }
                        else if (member.is_int16() || member.is_uint16())
                        {
                            u16* carray = (u16*)array;
                            for (s32 i = 0; i < count; ++i)
                            {
                                *carray++ = *((u16*)(&elem->m_ElemData64));
                                elem      = elem->m_Next;
                            }
                        }
                        else if (member.is_int32() || member.is_uint32() || member.is_f32())
                        {
                            u32* carray = (u32*)array;
                            for (s32 i = 0; i < count; ++i)
                            {
                                *carray++ = *((u32*)(&elem->m_ElemData64));
                                elem      = elem->m_Next;
                            }
                        }
                        else if (member.is_f64())
                        {
                            f64* carray = (f64*)array;
                            for (s32 i = 0; i < count; ++i)
                            {
                                *carray++ = *((f64*)(&elem->m_ElemData64));
                                elem      = elem->m_Next;
                            }
                        }
                    }
                    else if (member.is_string())
                    {
                        const char** carray = (char const**)array;
                        for (s32 i = 0; i < count; ++i)
                        {
                            *carray++ = (const char*)(elem->m_ElemData);
                            elem      = elem->m_Next;
                        }
                    }
                    else if (member.is_object())
                    {
                        for (s32 i = 0; i < count; ++i)
                        {
                            void* src = elem->m_ElemData;
                            member.m_descr->m_funcs->m_copy(array, i, src);
                            elem = elem->m_Next;
                        }
                    }
                    else
                    {
                        return MakeJsonError(json_state, "invalid array element type");
                    }
                }
                else
                {
                    // here we have an array of pointers to any of the types we support
                    void** parray = (void**)array;
                    for (s32 i = 0; i < count; ++i)
                    {
                        *parray++ = *(void**)(&elem->m_ElemData64);
                        elem      = elem->m_Next;
                    }
                }

                if (member.is_array_ptr())
                {
                    // set the vector pointer correctly for the member
                    void** ptr = (void**)member.get_member_ptr(object);
                    *ptr       = array;
                }
            }

            return nullptr;
        }

        JsonError* JsonDecodeValue(JsonState* json_state, JsonObject& object, JsonMember& member)
        {
            JsonError* err = nullptr;
            JsonLexeme l   = JsonLexerPeek(&json_state->m_Lexer);
            switch (l.m_Type)
            {
                case kJsonLexBeginObject:
                {
                    JsonObject member_object;
                    if (member.has_descr())
                    {
                        if (!member.is_object())
                            return MakeJsonError(json_state, "encountered json object but class member is not the same type");
                        member_object = member.get_object(object, json_state->m_Allocator);
                    }
                    json_state->m_NumberOfObjects += 1;
                    err = JsonDecodeObject(json_state, member_object);
                }
                break;
                case kJsonLexBeginArray:
                    if (member.has_descr() && (!member.is_array() && !member.is_array_ptr()))
                        return MakeJsonError(json_state, "encountered json array but class member is not the same type");

                    json_state->m_NumberOfArrays += 1;
                    err = JsonDecodeArray(json_state, object, member);
                    break;

                case kJsonLexString:
                {
                    if (member.has_descr() && !member.is_string())
                        return MakeJsonError(json_state, "encountered json string but class member is not the same type");

                    json_state->m_NumberOfStrings += 1;
                    member.set_string(object, l.m_String);

                    JsonLexerSkip(&json_state->m_Lexer);
                    break;
                }

                case kJsonLexNumber:
                {
                    if (member.has_descr() && !member.is_number())
                        return MakeJsonError(json_state, "encountered json number but class member is not the same type");

                    json_state->m_NumberOfNumbers += 1;
                    member.set_number(object, json_state->m_Allocator, l.m_Number);

                    JsonLexerSkip(&json_state->m_Lexer);
                    break;
                }

                case kJsonLexBoolean:
                    if (member.has_descr() && !member.is_bool())
                        return MakeJsonError(json_state, "encountered json boolean but class member is not the same type");

                    json_state->m_NumberOfBooleans += 1;
                    member.set_bool(object, json_state->m_Allocator, l.m_Boolean);

                    JsonLexerSkip(&json_state->m_Lexer);
                    break;

                case kJsonLexNull: JsonLexerSkip(&json_state->m_Lexer); break;

                default: return MakeJsonError(json_state, "invalid document");
            }

            return nullptr;
        }

        bool JsonDecode(char const* str, char const* end, JsonObject& json_root, JsonAllocator* allocator, JsonAllocator* scratch, char const*& error_message)
        {
            JsonAllocatorScope scratch_scope(scratch);

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

            error_message    = nullptr;
            s32 const len    = ascii::strlen(json_state->m_ErrorMessage);
            char*     errmsg = scratch->AllocateArray<char>(len + 1);
            x_memcopy(errmsg, json_state->m_ErrorMessage, len);
            errmsg[len]   = '\0';
            error_message = errmsg;
            return false;
        }

    } // namespace json_decoder

    namespace json
    {
        bool JsonDecode(char const* str, char const* end, JsonObject& json_root, JsonAllocator* allocator, JsonAllocator* scratch, char const*& error_message) { return json_decoder::JsonDecode(str, end, json_root, allocator, scratch, error_message); }
    } // namespace json
} // namespace xcore
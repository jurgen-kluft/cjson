#include "cbase/c_allocator.h"
#include "cbase/c_context.h"
#include "cbase/c_memory.h"
#include "cbase/c_printf.h"
#include "cbase/c_runes.h"
#include "cjson/c_json.h"
#include "cjson/c_json_utils.h"
#include "cjson/c_json_allocator.h"
#include "cjson/c_json_decode.h"
#include "cjson/c_json_lexer.h"

namespace ncore
{
    namespace json
    {
        static JsonSystemTypeDef sJsonTypeDescrBool("bool", sizeof(bool), alignof(bool));
        static JsonSystemTypeDef sJsonTypeDescrInt8("s8", sizeof(s8), alignof(s8));
        static JsonSystemTypeDef sJsonTypeDescrInt16("s16", sizeof(s16), alignof(s16));
        static JsonSystemTypeDef sJsonTypeDescrInt32("s32", sizeof(s32), alignof(s32));
        static JsonSystemTypeDef sJsonTypeDescrInt64("s64", sizeof(s64), alignof(s64));
        static JsonSystemTypeDef sJsonTypeDescrUInt8("u8", sizeof(u8), alignof(u8));
        static JsonSystemTypeDef sJsonTypeDescrUInt16("u16", sizeof(u16), alignof(u16));
        static JsonSystemTypeDef sJsonTypeDescrUInt32("u32", sizeof(u32), alignof(u32));
        static JsonSystemTypeDef sJsonTypeDescrUInt64("u64", sizeof(u64), alignof(u64));
        static JsonSystemTypeDef sJsonTypeDescrFloat32("f32", sizeof(f32), alignof(f32));
        static JsonSystemTypeDef sJsonTypeDescrFloat64("f64", sizeof(f64), alignof(f64));
        static JsonSystemTypeDef sJsonTypeDescrString("string", sizeof(const char*), alignof(const char*));
        static JsonSystemTypeDef sJsonTypeDescrEnum16("enum(u16)", sizeof(u16), alignof(u16));

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
        JsonTypeDescr const* JsonTypeDescrEnum16  = &sJsonTypeDescrEnum16;

        JsonObjectTypeDef* JsonTypeDescr::as_object_type() const
        {
            if (m_type == ObjectType)
            {
                return (JsonObjectTypeDef*)this;
            }
            return nullptr;
        }

        JsonEnumTypeDef* JsonTypeDescr::as_enum_type() const
        {
            if (m_type == EnumType)
            {
                return (JsonEnumTypeDef*)this;
            }
            return nullptr;
        }

        void default_copy_fn(void* _dst, void* _src, s16 _sizeof)
        {
            char*       dst = (char*)_dst;
            const char* src = (const char*)_src;
            for (s32 i = 0; i < _sizeof; ++i)
                dst[i] = src[i];
        }

        JsonCopyFn JsonTypeDescr::get_copy_fn() const
        {
            if (m_type == ObjectType)
            {
                JsonObjectTypeDef* object_type_def = (JsonObjectTypeDef*)this;
                return object_type_def->m_copy;
            }
            return nullptr;
        }

        JsonPlacementNewFn JsonTypeDescr::get_placement_new_fn() const
        {
            if (m_type == ObjectType)
            {
                JsonObjectTypeDef* object_type_def = (JsonObjectTypeDef*)this;
                return object_type_def->m_pnew;
            }
            return nullptr;
        }

        JsonObjectTypeDef::JsonObjectTypeDef(const char* _name, void* _default, s16 _sizeof, s16 _align_of, s32 _member_count, JsonFieldDescr* _members, JsonPlacementNewFn pnew, JsonCopyFn copy)
            : JsonTypeDescr(_name, _sizeof, _align_of, JsonTypeDescr::ObjectType)
            , m_default(_default)
            , m_member_count(_member_count)
            , m_members(_members)
            , m_pnew(pnew)
            , m_copy(copy)
        {
            if (m_copy == nullptr)
                m_copy = default_copy_fn;
        }

        JsonEnumTypeDef::JsonEnumTypeDef(const char* _name, s16 _sizeof, s16 _align_of, const char** _enum_strs, JsonEnumToStringFn _enum_to_str, JsonEnumFromStringFn _enum_from_str)
            : JsonTypeDescr(_name, _sizeof, _align_of, JsonTypeDescr::EnumType)
            , m_enum_strs(_enum_strs)
            , m_to_str(_enum_to_str)
            , m_from_str(_enum_from_str)
        {
            if (m_to_str == nullptr)
                m_to_str = EnumToString;
            if (m_from_str == nullptr)
                m_from_str = EnumFromString;
        }

        static void json_alloc_object(JsonTypeDescr const* descr, JsonAllocator* alloc, s32 n, void*& ptr)
        {
            JsonPlacementNewFn pnew = descr->get_placement_new_fn();

            ptr       = alloc->Allocate(n * descr->m_sizeof, descr->m_alignof);
            char* mem = (char*)ptr;
            for (s32 i = 0; i < n; ++i)
            {
                void* p = mem + i * descr->m_sizeof;
                if (pnew != nullptr)
                    pnew(p);
            }
        }

        void* JsonMember::get_member_ptr(JsonObject const& object)
        {
            JsonObjectTypeDef const* o = object.m_descr->as_object_type();
            ASSERT(o != nullptr);
            ptr_t offset = (ptr_t)m_descr->m_member - (ptr_t)o->m_default;
            return (void*)((ptr_t)object.m_instance + offset);
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
                    json_alloc_object(m_descr->m_typedescr, alloc, 1, o.m_instance);
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

        void JsonMember::set_enum(JsonObject const& object, const char* str)
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
                    ASSERT(false); // enum is not supported for arrays (yet)
                    // The member value is part of something else, like an array element
                    // The caller has provided a pointer to the member value.
                }

                ASSERT(is_enum());

                JsonEnumTypeDef* etd = m_descr->m_typedescr->as_enum_type();

                u64 value = 0;
                etd->m_from_str(str, etd->m_enum_strs, value);

                if (is_enum16())
                    *((u16*)m_data_ptr) = (u16)value;
                else if (is_enum32())
                    *((u32*)m_data_ptr) = (u32)value;
                else if (is_enum64())
                    *((u64*)m_data_ptr) = (u64)value;
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
                        void*  value_ptr        = alloc->Allocate(m_descr->m_typedescr->m_sizeof, m_descr->m_typedescr->m_alignof);
                        *member_value_ptr       = value_ptr;
                        m_data_ptr              = value_ptr;
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
                        void*  value_ptr        = alloc->Allocate(m_descr->m_typedescr->m_sizeof, m_descr->m_typedescr->m_alignof);
                        *member_value_ptr       = value_ptr;
                        m_data_ptr              = value_ptr;
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
                JsonObjectTypeDef* objdef = m_descr->as_object_type();

                for (s32 i = 0; i < objdef->m_member_count; ++i)
                {
                    const char* n1 = name;
                    const char* n2 = objdef->m_members[i].m_name;
                    while (true)
                    {
                        if (*n1 != *n2)
                            break;

                        if (*n1 == 0)
                        {
                            member.m_descr = &objdef->m_members[i];
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
            JsonLexerState m_Lexer;
            char*          m_ErrorMessage;
            JsonAllocator* m_Allocator;
            JsonAllocator* m_Scratch;
            int            m_NumberOfObjects;
            int            m_NumberOfNumbers;
            int            m_NumberOfStrings;
            int            m_NumberOfEnums;
            int            m_NumberOfArrays;
            int            m_NumberOfBooleans;
        };

        static void JsonStateInit(JsonState* state, JsonAllocator* alloc, JsonAllocator* scratch, char const* buffer, char const* end)
        {
            JsonLexerStateInit(&state->m_Lexer, buffer, end, alloc, scratch);
            state->m_ErrorMessage     = nullptr;
            state->m_Allocator        = alloc;
            state->m_Scratch          = scratch;
            state->m_NumberOfObjects  = 0;
            state->m_NumberOfNumbers  = 0;
            state->m_NumberOfStrings  = 0;
            state->m_NumberOfEnums    = 0;
            state->m_NumberOfArrays   = 0;
            state->m_NumberOfBooleans = 2;
        }

        struct JsonError
        {
            const char* m_ErrorMessage;
        };

        static JsonError* MakeJsonError(JsonState* state, const char* error)
        {
            state->m_ErrorMessage = state->m_Allocator->AllocateArray<char>(1024);
            runes_t  errmsg = ascii::make_runes(state->m_ErrorMessage, state->m_ErrorMessage + 1024 - 1);
            crunes_t fmt = make_crunes("line %d: %s");
            sprintf(errmsg, fmt, va_t(state->m_Lexer.m_LineNumber), va_t(error));
            return nullptr;
        }

        static JsonError* JsonDecodeValue(JsonState* json_state, JsonObject& object, JsonMember& member);

        static JsonError* JsonDecodeObject(JsonState* json_state, JsonObject& object)
        {
            JsonLexerState* lexer = &json_state->m_Lexer;

            if (!JsonLexerExpect(lexer, kJsonLexBeginObject))
                return MakeJsonError(json_state, "expected '{'");

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
                        json_alloc_object(m.m_descr->m_typedescr, scratch, 1, obj);
                        elem->m_ElemData = obj;
                        m.m_data_ptr     = obj;
                    }
                    err = JsonDecodeValue(json_state, object, m);
                    value_list.Add(elem);
                }
                else
                {
                    ListElem* elem = value_list.NewListItem();
                    err            = JsonDecodeValue(json_state, object, member);
                    value_list.Add(elem);
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
                    JsonObjectTypeDef const* obj_type_def = object.m_descr->as_object_type();

                    if (member.is_array_ptr_size8())
                    {
                        if (count > 127)
                            count = 127;

                        ptr_t const offset = (ptr_t)member.m_descr->m_size8 - (ptr_t)obj_type_def->m_default;
                        s8*        size8  = (s8*)((ptr_t)object.m_instance + offset);
                        *size8            = (s8)count;
                    }
                    else if (member.is_array_ptr_size16())
                    {
                        if (count > 32767)
                            count = 32767;
                        ptr_t const offset = (ptr_t)member.m_descr->m_size16 - (ptr_t)obj_type_def->m_default;
                        s16*       size16 = (s16*)((ptr_t)object.m_instance + offset);
                        *size16           = (s16)count;
                    }
                    else if (member.is_array_ptr_size32())
                    {
                        if (count > 2147483647)
                            count = 2147483647;
                        ptr_t const offset = (ptr_t)member.m_descr->m_size32 - (ptr_t)obj_type_def->m_default;
                        s32*       size32 = (s32*)((ptr_t)object.m_instance + offset);
                        *size32           = count;
                    }
                    array = alloc->Allocate(count * (member.m_descr->m_typedescr->m_sizeof), member.m_descr->m_typedescr->m_alignof);
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
                        char*              a        = (char*)array;
                        s16 const          s        = member.m_descr->m_typedescr->m_sizeof;
                        JsonObjectTypeDef* obj_type = member.m_descr->m_typedescr->as_object_type();
                        for (s32 i = 0; i < count; ++i)
                        {
                            void* src = elem->m_ElemData;
                            if (obj_type->m_copy != nullptr)
                            {
                                obj_type->m_copy(a, src, s);
                            }
                            a += s;
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
                    if (member.has_descr() && (!member.is_string() && !member.is_enum()))
                        return MakeJsonError(json_state, "encountered json string but class member is not the same type");

                    if (member.has_descr())
                    {
                        if (member.is_string())
                        {
                            json_state->m_NumberOfStrings += 1;
                            member.set_string(object, l.m_String);
                        }
                        else if (member.is_enum())
                        {
                            json_state->m_NumberOfEnums += 1;
                            member.set_enum(object, l.m_String);
                        }
                    }
                    else
                    {
                        json_state->m_NumberOfStrings += 1;
                    }

                    JsonLexerSkip(&json_state->m_Lexer);
                    break;
                }

                case kJsonLexNumber:
                {
                    if (member.has_descr() && !member.is_number())
                        return MakeJsonError(json_state, "encountered json number but class member is not the same type");

                    json_state->m_NumberOfNumbers += 1;
                    if (member.has_descr())
                    {
                        member.set_number(object, json_state->m_Allocator, l.m_Number);
                    }

                    JsonLexerSkip(&json_state->m_Lexer);
                    break;
                }

                case kJsonLexBoolean:
                    if (member.has_descr() && !member.is_bool())
                        return MakeJsonError(json_state, "encountered json boolean but class member is not the same type");

                    json_state->m_NumberOfBooleans += 1;
                    if (member.has_descr())
                    {
                        member.set_bool(object, json_state->m_Allocator, l.m_Boolean);
                    }

                    JsonLexerSkip(&json_state->m_Lexer);
                    break;

                case kJsonLexNull: JsonLexerSkip(&json_state->m_Lexer); break;

                default: return MakeJsonError(json_state, "invalid document");
            }

            return err;
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
            nmem::memcpy(errmsg, json_state->m_ErrorMessage, len);
            errmsg[len]   = '\0';
            error_message = errmsg;
            return false;
        }

    } // namespace json_decoder

    namespace json
    {
        bool JsonDecode(char const* str, char const* end, JsonObject& json_root, JsonAllocator* allocator, JsonAllocator* scratch, char const*& error_message) { return json_decoder::JsonDecode(str, end, json_root, allocator, scratch, error_message); }
    } // namespace json
} // namespace ncore

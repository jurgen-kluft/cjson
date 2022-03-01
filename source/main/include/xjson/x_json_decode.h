#ifndef __XBASE_JSON_DECODE_H__
#define __XBASE_JSON_DECODE_H__
#include "xbase/x_target.h"
#ifdef USE_PRAGMA_ONCE
#pragma once
#endif

namespace xcore
{
    namespace json
    {
        struct JsonAllocator;

        typedef void (*JsonAllocatorFn)(JsonAllocator* alloc, s32 count, void*& p);

        struct JsonObjectDescr;

        struct JsonMemberType
        {
            enum
            {
                TypeNull    = 0x00000,
                TypeBool    = 0x00001,
                TypeInt8    = 0x00002,
                TypeInt16   = 0x00004,
                TypeInt32   = 0x00008,
                TypeInt64   = 0x00010,
                TypeF32     = 0x00020,
                TypeF64     = 0x00040,
                TypeString  = 0x00080,
                TypeObject  = 0x00100,
                TypePointer = 0x00200,
                TypeVector  = 0x00400,
                TypeCarray  = 0x00800,
                TypeSize8   = 0x01000,
                TypeSize16  = 0x02000,
                TypeSize32  = 0x04000,
            };

            xcore::u32 m_type;

            inline bool is_bool() const { return (m_type & TypeBool) == TypeBool; }
            inline bool is_int8() const { return (m_type & TypeInt8) == TypeInt8; }
            inline bool is_int16() const { return (m_type & TypeInt16) == TypeInt16; }
            inline bool is_int32() const { return (m_type & TypeInt32) == TypeInt32; }
            inline bool is_int64() const { return (m_type & TypeInt64) == TypeInt64; }
            inline bool is_f32() const { return (m_type & TypeF32) == TypeF32; }
            inline bool is_f64() const { return (m_type & TypeF64) == TypeF64; }
            inline bool is_string() const { return (m_type & TypeString) == TypeString; }
            inline bool is_object() const { return (m_type & TypeObject) == TypeObject; }
            inline bool is_pointer() const { return (m_type & TypePointer) == TypePointer; }
            inline bool is_vector() const { return (m_type & TypeVector) == TypeVector; }
            inline bool is_carray() const { return (m_type & TypeCarray) == TypeCarray; }

            JsonAllocatorFn  m_alloc;
            JsonObjectDescr* m_object;
        };

        JsonMemberType const* JsonTypeBool;
        JsonMemberType const* JsonTypeBoolPtr;
        JsonMemberType const* JsonTypeBoolVector;
        JsonMemberType const* JsonTypeBoolCArray;

        JsonMemberType const* JsonTypeInt8;
        JsonMemberType const* JsonTypeInt8Ptr;
        JsonMemberType const* JsonTypeInt8Vector;
        JsonMemberType const* JsonTypeInt8CArray;

        JsonMemberType const* JsonTypeInt16;
        JsonMemberType const* JsonTypeInt16Ptr;
        JsonMemberType const* JsonTypeInt16Vector;
        JsonMemberType const* JsonTypeInt16CArray;

        JsonMemberType const* JsonTypeInt32;
        JsonMemberType const* JsonTypeInt32Ptr;
        JsonMemberType const* JsonTypeInt32Vector;
        JsonMemberType const* JsonTypeInt32CArray;

        JsonMemberType const* JsonTypeInt64;
        JsonMemberType const* JsonTypeInt64Ptr;
        JsonMemberType const* JsonTypeInt64Vector;
        JsonMemberType const* JsonTypeInt64CArray;

        JsonMemberType const* JsonTypeFloat32;
        JsonMemberType const* JsonTypeFloat32Ptr;
        JsonMemberType const* JsonTypeFloat32Vector;
        JsonMemberType const* JsonTypeFloat32Vector8;
        JsonMemberType const* JsonTypeFloat32Vector16;
        JsonMemberType const* JsonTypeFloat32CArray;

        JsonMemberType const* JsonTypeFloat64;
        JsonMemberType const* JsonTypeFloat64Ptr;
        JsonMemberType const* JsonTypeFloat64Vector;
        JsonMemberType const* JsonTypeFloat64CArray;

        JsonMemberType const* JsonTypeString;
        JsonMemberType const* JsonTypeStringPtr;
        JsonMemberType const* JsonTypeStringVector;
        JsonMemberType const* JsonTypeStringCArray;

        struct JsonMemberDescr
        {
            JsonMemberDescr(const char* name, bool* member)
                : m_type(JsonTypeBool)
                , m_size32(nullptr)
                , m_name(name)
                , m_member(member)
            {
            }
            JsonMemberDescr(const char* name, bool** member)
                : m_type(JsonTypeBoolPtr)
                , m_size32(nullptr)
                , m_name(name)
                , m_member(member)
            {
            }

            JsonMemberDescr(const char* name, bool** member, s32 count)
                : m_type(JsonTypeBoolCArray)
                , m_csize(count)
                , m_name(name)
                , m_member(member)
            {
            }

            JsonMemberDescr(const char* name, bool** member, s32* count)
                : m_type(JsonTypeBoolVector)
                , m_size32(count)
                , m_name(name)
                , m_member(member)
            {
            }

            JsonMemberDescr(const char* name, xcore::s8* member)
                : m_type(JsonTypeInt8)
                , m_size32(nullptr)
                , m_name(name)
                , m_member(member)
            {
            }

            JsonMemberDescr(const char* name, xcore::s8** member)
                : m_type(JsonTypeInt8Ptr)
                , m_size32(nullptr)
                , m_name(name)
                , m_member(member)
            {
            }

            JsonMemberDescr(const char* name, xcore::s8** member, s32 count)
                : m_type(JsonTypeInt8CArray)
                , m_csize(count)
                , m_name(name)
                , m_member(member)
            {
            }

            JsonMemberDescr(const char* name, xcore::s8** member, s32* count)
                : m_type(JsonTypeInt8Vector)
                , m_size32(count)
                , m_name(name)
                , m_member(member)
            {
            }

            JsonMemberDescr(const char* name, xcore::s16* member)
                : m_type(JsonTypeInt16)
                , m_size32(nullptr)
                , m_name(name)
                , m_member(member)
            {
            }

            JsonMemberDescr(const char* name, xcore::s16** member)
                : m_type(JsonTypeInt16Ptr)
                , m_size32(nullptr)
                , m_name(name)
                , m_member(member)
            {
            }

            JsonMemberDescr(const char* name, xcore::s16** member, s32 count)
                : m_type(JsonTypeInt16CArray)
                , m_csize(count)
                , m_name(name)
                , m_member(member)
            {
            }

            JsonMemberDescr(const char* name, xcore::s16** member, s32* count)
                : m_type(JsonTypeInt16Vector)
                , m_size32(count)
                , m_name(name)
                , m_member(member)
            {
            }

            JsonMemberDescr(const char* name, xcore::s32* member)
                : m_type(JsonTypeInt32)
                , m_size32(nullptr)
                , m_name(name)
                , m_member(member)
            {
            }

            JsonMemberDescr(const char* name, xcore::s32** member)
                : m_type(JsonTypeInt32Ptr)
                , m_size32(nullptr)
                , m_name(name)
                , m_member(member)
            {
            }

            JsonMemberDescr(const char* name, xcore::s32** member, s32 count)
                : m_type(JsonTypeInt32CArray)
                , m_csize(count)
                , m_name(name)
                , m_member(member)
            {
            }

            JsonMemberDescr(const char* name, xcore::s32** member, s32* count)
                : m_type(JsonTypeInt32Vector)
                , m_size32(count)
                , m_name(name)
                , m_member(member)
            {
            }

            JsonMemberDescr(const char* name, float* member)
                : m_type(JsonTypeFloat32)
                , m_size32(nullptr)
                , m_name(name)
                , m_member(member)
            {
            }

            template <s32 N>
            JsonMemberDescr(const char* name, float (*member)[N], s32 count)
                : m_type(JsonTypeFloatCArray)
                , m_csize(count)
                , m_name(name)
                , m_member(member)
            {
            }

            JsonMemberDescr(const char* name, float** member)
                : m_type(JsonTypeFloat32Ptr)
                , m_size32(nullptr)
                , m_name(name)
                , m_member(member)
            {
            }

            JsonMemberDescr(const char* name, float** member, s32 count)
                : m_type(JsonTypeFloat32CArray)
                , m_csize(count)
                , m_name(name)
                , m_member(member)
            {
            }

            JsonMemberDescr(const char* name, float** member, s8* count)
                : m_type(JsonTypeFloat32Vector8)
                , m_size8(count)
                , m_name(name)
                , m_member(member)
            {
            }

            JsonMemberDescr(const char* name, float** member, s16* count)
                : m_type(JsonTypeFloat32Vector16)
                , m_size16(count)
                , m_name(name)
                , m_member(member)
            {
            }

            JsonMemberDescr(const char* name, float** member, s32* count)
                : m_type(JsonTypeFloat32Vector)
                , m_size32(count)
                , m_name(name)
                , m_member(member)
            {
            }

            template <s32 N>
            JsonMemberDescr(const char* name, float* (*member)[N], s32 count = N)
                : m_type(JsonTypeFloat32Vector)
                , m_size32(count)
                , m_name(name)
                , m_member(member)
            {
            }

            JsonMemberDescr(const char* name, const char** member)
                : m_type(JsonTypeString)
                , m_size32(nullptr)
                , m_name(name)
                , m_member(member)
            {
            }

            JsonMemberDescr(const char* name, void* member, JsonMemberType* objectType)
                : m_type(objectType)
                , m_name(name)
                , m_size32(nullptr)
                , m_member(member)
            {
                // e.g. key_t m_key;
            }

            JsonMemberDescr(const char* name, void** member, JsonMemberType* objectType)
                : m_type(objectType)
                , m_name(name)
                , m_size32(nullptr)
                , m_member(member)
            {
                // e.g. key_t* m_key;
            }

            template <typename T>
            JsonMemberDescr(const char* name, T** member, xcore::s16* count, JsonMemberType* objectType)
                : m_type(objectType)
                , m_name(name)
                , m_size16(count)
                , m_member(member)
            {
                // e.g.
                //  - key_t* m_keys; // array of keys (size<32768)
            }

            template <typename T>
            JsonMemberDescr(const char* name, T** member, xcore::s32* count, JsonMemberType* objectType)
                : m_type(objectType)
                , m_name(name)
                , m_size32(count)
                , m_member(member)
            {
                // e.g.
                //  - key_t* m_keys; // array of keys (size<2147483647)
            }

            JsonMemberType const* m_type;
            const char*           m_name;
            void*                 m_member;
            union
            {
                xcore::s32  m_csize;
                xcore::s8*  m_size8;
                xcore::s16* m_size16;
                xcore::s32* m_size32;
            };
        };

        // Defines a custom JSON object type
        struct JsonObjectDescr
        {
            const char*      m_name;
            void*            m_default;
            int              m_member_count;
            JsonMemberDescr* m_members; // Sorted by name
        };

        struct JsonMember
        {
            JsonMemberDescr const* m_member_descr;
            void*                  m_instance;
        };

        struct JsonObject
        {
            JsonObjectDescr const* m_object_descr;
            void*                  m_instance;
        };

        JsonMember JsonObjectGetMember(JsonObject& object, const char* name);
        void* JsonObjectGetMemberPtr(JsonObject& object, JsonMember& member);

        bool JsonDecode(char const* json, char const* json_end, JsonObject* json_root, void* root, JsonAllocator* allocator, JsonAllocator* scratch, char const*& error_message);

    } // namespace json
} // namespace xcore

#endif // __XBASE_JSON_H__
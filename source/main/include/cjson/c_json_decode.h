#ifndef __CJSON_JSON_DECODE_H__
#define __CJSON_JSON_DECODE_H__
#include "ccore/c_target.h"
#ifdef USE_PRAGMA_ONCE
#    pragma once
#endif

namespace ncore
{
    namespace njson
    {
        struct JsonAllocator;

        typedef void (*JsonAllocatorFn)(JsonAllocator* alloc, s32 count, void*& p);
        typedef void (*JsonPlacementNewFn)(void* dst);
        typedef void (*JsonCopyFn)(void* _dst, void* _src, s16 _sizeof);
        typedef void (*JsonEnumToStringFn)(u64 in_enum, const char** enum_strs, const u64* enum_values, i32 enum_count, char*& out_str, char const* out_end);
        typedef void (*JsonEnumFromStringFn)(const char*& in_str, const char** enum_strs, const u64* enum_values, i32 enum_count, u64& out_enum);
        typedef void (*JsonFlagsToStringFn)(u64 in_enum, const char** enum_strs, const u64* enum_values, i32 enum_count, char*& out_str, char const* out_end);
        typedef void (*JsonFlagsFromStringFn)(const char*& in_str, const char** enum_strs, const u64* enum_values, i32 enum_count, u64& out_enum);

        struct JsonType
        {
            enum
            {
                TypeNull     = 0x00000,
                TypeUInt8    = 0x00001,
                TypeUInt16   = 0x00002,
                TypeUInt32   = 0x00004,
                TypeUInt64   = 0x00008,
                TypeF32      = 0x00010,
                TypeF64      = 0x00020,
                TypeBool     = 0x00040,
                TypeSigned   = 0x00080,
                TypeInt8     = TypeUInt8 | TypeSigned,
                TypeInt16    = TypeUInt16 | TypeSigned,
                TypeInt32    = TypeUInt32 | TypeSigned,
                TypeInt64    = TypeUInt64 | TypeSigned,
                TypeNumber   = TypeUInt8 | TypeUInt16 | TypeUInt32 | TypeUInt64 | TypeSigned | TypeF32 | TypeF64,
                TypeString   = 0x00100,
                TypeObject   = 0x00200,
                TypePointer  = 0x00400,
                TypeEnum16   = 0x00800,
                TypeEnum32   = 0x01000,
                TypeEnum64   = 0x02000,
                TypeEnum     = TypeEnum16 | TypeEnum32 | TypeEnum64,
                TypeArray    = 0x04000,
                TypeArrayPtr = 0x08000,
                TypeSize8    = 0x10000,
                TypeSize16   = 0x20000,
                TypeSize32   = 0x40000,
            };
        };

        struct JsonFieldDescr;

        class JsonSystemTypeDef;
        class JsonObjectTypeDef;
        class JsonEnumTypeDef;

        class JsonTypeDescr
        {
        public:
            JsonTypeDescr(const char* _name, s16 _sizeof, s16 _align_of, s32 _type)
                : m_name(_name)
                , m_sizeof(_sizeof)
                , m_alignof(_align_of)
                , m_type(_type)
            {
            }
            enum
            {
                SystemType = 0,
                ObjectType = 1,
                EnumType   = 2,
            };

            const char* m_name;
            s16         m_sizeof;
            s16         m_alignof;
            s32         m_type;

            JsonObjectTypeDef* as_object_type() const;
            JsonEnumTypeDef*   as_enum_type() const;

            JsonCopyFn         get_copy_fn() const;
            JsonPlacementNewFn get_placement_new_fn() const;
        };

        class JsonObjectTypeDef : public JsonTypeDescr
        {
        public:
            JsonObjectTypeDef(const char* _name, void* _default, s16 _sizeof, s16 _align_of, s32 _member_count, JsonFieldDescr* _members, JsonPlacementNewFn pnew, JsonCopyFn copy);
            void*              m_default;
            s32                m_member_count;
            JsonFieldDescr*    m_members;
            JsonPlacementNewFn m_pnew;
            JsonCopyFn         m_copy;
        };

        template <typename T> void JsonObjectTypeRegisterFields(T& base, JsonFieldDescr*& members, s32& member_count) {}
        template <typename T> class JsonObjectTypeDeclr : public JsonObjectTypeDef
        {
        public:
            static T& default_object()
            {
                static T default_instance;
                return default_instance;
            }
            static void placement_new(void* ptr) { new (ptr) T(); }

            JsonObjectTypeDeclr(const char* name)
                : JsonObjectTypeDef(name, &default_object(), sizeof(T), alignof(T), 0, nullptr, placement_new, nullptr)
            {
                JsonObjectTypeRegisterFields<T>(default_object(), m_members, m_member_count);
            }
        };

        // Enumeration type, stored as u16, u32 or u64
        class JsonEnumTypeDef : public JsonTypeDescr
        {
        public:
            JsonEnumTypeDef(const char* _name, s16 _sizeof, s16 _align_of, const char** _enum_strs, const u64* _enum_values, i32 enum_count);
            JsonEnumTypeDef(const char* _name, s16 _sizeof, s16 _align_of, JsonEnumToStringFn _enum_to_str, JsonEnumFromStringFn _enum_from_str);
            JsonEnumTypeDef(const char* _name, s16 _sizeof, s16 _align_of, const char** _enum_strs, const u64* _enum_values, i32 enum_count, JsonEnumToStringFn _enum_to_str, JsonEnumFromStringFn _enum_from_str);
            i32                  m_enum_count;
            const char**         m_enum_strs;
            const u64*           m_enum_values; // TODO use this
            JsonEnumToStringFn   m_to_str;
            JsonEnumFromStringFn m_from_str;
        };

        // Flags are like Enums, but can be combined using bitwise OR
        class JsonFlagsTypeDef : public JsonEnumTypeDef
        {
        public:
            JsonFlagsTypeDef(const char* _name, s16 _sizeof, s16 _align_of, const char** _enum_strs, const u64* _enum_values, i32 enum_count);
            JsonFlagsTypeDef(const char* _name, s16 _sizeof, s16 _align_of, JsonFlagsToStringFn _enum_to_str, JsonFlagsFromStringFn _enum_from_str);
            JsonFlagsTypeDef(const char* _name, s16 _sizeof, s16 _align_of, const char** _enum_strs, const u64* _enum_values, i32 enum_count, JsonFlagsToStringFn _enum_to_str, JsonFlagsFromStringFn _enum_from_str);
        };

        extern JsonTypeDescr const* JsonTypeDescrBool;
        extern JsonTypeDescr const* JsonTypeDescrInt8;
        extern JsonTypeDescr const* JsonTypeDescrInt16;
        extern JsonTypeDescr const* JsonTypeDescrInt32;
        extern JsonTypeDescr const* JsonTypeDescrInt64;
        extern JsonTypeDescr const* JsonTypeDescrUInt8;
        extern JsonTypeDescr const* JsonTypeDescrUInt16;
        extern JsonTypeDescr const* JsonTypeDescrUInt32;
        extern JsonTypeDescr const* JsonTypeDescrUInt64;
        extern JsonTypeDescr const* JsonTypeDescrFloat32;
        extern JsonTypeDescr const* JsonTypeDescrFloat64;
        extern JsonTypeDescr const* JsonTypeDescrString;
        extern JsonTypeDescr const* JsonTypeDescrEnum16;

        // There are only a handfull of JSON value types:
        // - string
        // - number (integer, double, bool)
        // - array
        // - map
        // - object
        // But we can have a JsonField and Value with a specific conversion:
        // - IPv4       -> u32
        // - MacAddress -> u64
        // - GUID       -> u8[N]
        struct JsonValueDescr
        {
        };

        struct JsonFieldDescr
        {
            u32                  m_type;
            JsonTypeDescr const* m_typedescr;
            const char*          m_name;
            void*                m_member;

            union
            {
                s32  m_csize;
                s8*  m_size8;
                s16* m_size16;
                s32* m_size32;
            };

            JsonFieldDescr(const char* name, bool& member)
                : m_type(JsonType::TypeBool)
                , m_typedescr(JsonTypeDescrBool)
                , m_name(name)
                , m_member(&member)
                , m_size32(nullptr)
            {
            }
            JsonFieldDescr(const char* name, bool*& member)
                : m_type(JsonType::TypeBool | JsonType::TypePointer)
                , m_typedescr(JsonTypeDescrBool)
                , m_name(name)
                , m_member(&member)
                , m_size32(nullptr)
            {
            }

            JsonFieldDescr(const char* name, bool*& member, s32 count)
                : m_type(JsonType::TypeBool | JsonType::TypeArray)
                , m_typedescr(JsonTypeDescrBool)
                , m_name(name)
                , m_member(&member)
                , m_csize(count)
            {
            }

            JsonFieldDescr(const char* name, bool*& member, s32& count)
                : m_type(JsonType::TypeBool | JsonType::TypeArrayPtr | JsonType::TypeSize32)
                , m_typedescr(JsonTypeDescrBool)
                , m_name(name)
                , m_member(&member)
                , m_size32(&count)
            {
            }

            JsonFieldDescr(const char* name, s8& member)
                : m_type(JsonType::TypeInt8)
                , m_typedescr(JsonTypeDescrInt8)
                , m_name(name)
                , m_member(&member)
                , m_size32(nullptr)
            {
            }

            JsonFieldDescr(const char* name, s8*& member)
                : m_type(JsonType::TypeInt8 | JsonType::TypePointer)
                , m_typedescr(JsonTypeDescrInt8)
                , m_name(name)
                , m_member(&member)
                , m_size32(nullptr)
            {
            }

            JsonFieldDescr(const char* name, s8*& member, s32 count)
                : m_type(JsonType::TypeInt8 | JsonType::TypeArray)
                , m_typedescr(JsonTypeDescrInt8)
                , m_name(name)
                , m_member(&member)
                , m_csize(count)
            {
            }

            JsonFieldDescr(const char* name, u8*& member, s8& count)
                : m_type(JsonType::TypeUInt8 | JsonType::TypeArrayPtr | JsonType::TypeSize8)
                , m_typedescr(JsonTypeDescrUInt8)
                , m_name(name)
                , m_member(&member)
                , m_size8(&count)
            {
            }

            template <s32 N>
            JsonFieldDescr(const char* name, u8 (&member)[N], s32 count)
                : m_type(JsonType::TypeUInt8 | JsonType::TypeArray)
                , m_typedescr(JsonTypeDescrUInt8)
                , m_name(name)
                , m_member(&member)
                , m_csize(count)
            {
            }

            JsonFieldDescr(const char* name, s8*& member, s32& count)
                : m_type(JsonType::TypeInt8 | JsonType::TypeArrayPtr | JsonType::TypeSize32)
                , m_typedescr(JsonTypeDescrInt8)
                , m_name(name)
                , m_member(&member)
                , m_size32(&count)
            {
            }

            JsonFieldDescr(const char* name, s16& member)
                : m_type(JsonType::TypeInt16)
                , m_typedescr(JsonTypeDescrInt16)
                , m_name(name)
                , m_member(&member)
                , m_size32(nullptr)
            {
            }

            JsonFieldDescr(const char* name, s16*& member)
                : m_type(JsonType::TypeInt16 | JsonType::TypePointer)
                , m_typedescr(JsonTypeDescrInt16)
                , m_name(name)
                , m_member(&member)
                , m_size32(nullptr)
            {
            }

            JsonFieldDescr(const char* name, s16*& member, s32 count)
                : m_type(JsonType::TypeInt16 | JsonType::TypeArray)
                , m_typedescr(JsonTypeDescrInt16)
                , m_name(name)
                , m_member(&member)
                , m_csize(count)
            {
            }

            JsonFieldDescr(const char* name, s16*& member, s32& count)
                : m_type(JsonType::TypeInt16 | JsonType::TypeArrayPtr | JsonType::TypeSize32)
                , m_typedescr(JsonTypeDescrInt16)
                , m_name(name)
                , m_member(&member)
                , m_size32(&count)
            {
            }

            JsonFieldDescr(const char* name, u16& member)
                : m_type(JsonType::TypeUInt16)
                , m_typedescr(JsonTypeDescrUInt16)
                , m_name(name)
                , m_member(&member)
                , m_size32(nullptr)
            {
            }

            JsonFieldDescr(const char* name, u32& member)
                : m_type(JsonType::TypeUInt32)
                , m_typedescr(JsonTypeDescrUInt32)
                , m_name(name)
                , m_member(&member)
                , m_size32(nullptr)
            {
            }

            JsonFieldDescr(const char* name, u64& member)
                : m_type(JsonType::TypeUInt64)
                , m_typedescr(JsonTypeDescrUInt64)
                , m_name(name)
                , m_member(&member)
                , m_size32(nullptr)
            {
            }

            JsonFieldDescr(const char* name, u16*& member)
                : m_type(JsonType::TypeUInt16 | JsonType::TypePointer)
                , m_typedescr(JsonTypeDescrUInt16)
                , m_name(name)
                , m_member(&member)
                , m_size32(nullptr)
            {
            }

            JsonFieldDescr(const char* name, u16& member, JsonEnumTypeDef& enumtype)
                : m_type(JsonType::TypeUInt16 | JsonType::TypeEnum16)
                , m_typedescr(&enumtype)
                , m_name(name)
                , m_member(&member)
                , m_size32(nullptr)
            {
            }

            JsonFieldDescr(const char* name, u16*& member, s32 count)
                : m_type(JsonType::TypeUInt16 | JsonType::TypeArray)
                , m_typedescr(JsonTypeDescrUInt16)
                , m_name(name)
                , m_member(&member)
                , m_csize(count)
            {
            }

            JsonFieldDescr(const char* name, u16*& member, s32& count)
                : m_type(JsonType::TypeUInt16 | JsonType::TypeArrayPtr | JsonType::TypeSize32)
                , m_typedescr(JsonTypeDescrUInt16)
                , m_name(name)
                , m_member(&member)
                , m_size32(&count)
            {
            }

            JsonFieldDescr(const char* name, s32& member)
                : m_type(JsonType::TypeInt32)
                , m_typedescr(JsonTypeDescrInt32)
                , m_name(name)
                , m_member(&member)
                , m_size32(nullptr)
            {
            }

            JsonFieldDescr(const char* name, s32*& member)
                : m_type(JsonType::TypeInt32 | JsonType::TypePointer)
                , m_typedescr(JsonTypeDescrInt32)
                , m_name(name)
                , m_member(&member)
                , m_size32(nullptr)
            {
            }

            JsonFieldDescr(const char* name, s32*& member, s32 count)
                : m_type(JsonType::TypeInt32 | JsonType::TypeArray)
                , m_typedescr(JsonTypeDescrInt32)
                , m_name(name)
                , m_member(&member)
                , m_csize(count)
            {
            }

            JsonFieldDescr(const char* name, s32*& member, s32& count)
                : m_type(JsonType::TypeInt32 | JsonType::TypeArrayPtr | JsonType::TypeSize32)
                , m_typedescr(JsonTypeDescrInt32)
                , m_name(name)
                , m_member(&member)
                , m_size32(&count)
            {
            }

            JsonFieldDescr(const char* name, float& member)
                : m_type(JsonType::TypeF32)
                , m_typedescr(JsonTypeDescrFloat32)
                , m_name(name)
                , m_member(&member)
                , m_size32(nullptr)
            {
            }

            template <s32 N>
            JsonFieldDescr(const char* name, float (&member)[N], s32 count)
                : m_type(JsonType::TypeF32 | JsonType::TypeArray)
                , m_typedescr(JsonTypeDescrFloat32)
                , m_name(name)
                , m_member(&member)
                , m_csize(count)
            {
            }

            JsonFieldDescr(const char* name, float*& member)
                : m_type(JsonType::TypeF32 | JsonType::TypePointer)
                , m_typedescr(JsonTypeDescrFloat32)
                , m_name(name)
                , m_member(&member)
                , m_size32(nullptr)
            {
            }

            JsonFieldDescr(const char* name, float*& member, s32 count)
                : m_type(JsonType::TypeF32 | JsonType::TypeArray)
                , m_typedescr(JsonTypeDescrFloat32)
                , m_name(name)
                , m_member(&member)
                , m_csize(count)
            {
            }

            JsonFieldDescr(const char* name, float*& member, s8& count)
                : m_type(JsonType::TypeF32 | JsonType::TypeArrayPtr | JsonType::TypeSize8)
                , m_typedescr(JsonTypeDescrFloat32)
                , m_name(name)
                , m_member(&member)
                , m_size8(&count)
            {
            }

            JsonFieldDescr(const char* name, float*& member, s16& count)
                : m_type(JsonType::TypeF32 | JsonType::TypeArrayPtr | JsonType::TypeSize16)
                , m_typedescr(JsonTypeDescrFloat32)
                , m_name(name)
                , m_member(&member)
                , m_size16(&count)
            {
            }

            JsonFieldDescr(const char* name, float*& member, s32& count)
                : m_type(JsonType::TypeF32 | JsonType::TypeArrayPtr | JsonType::TypeSize32)
                , m_typedescr(JsonTypeDescrFloat32)
                , m_name(name)
                , m_member(&member)
                , m_size32(&count)
            {
            }

            template <s32 N>
            JsonFieldDescr(const char* name, float* (&member)[N], s32 count = N)
                : m_type(JsonType::TypeF32 | JsonType::TypeArray)
                , m_typedescr(JsonTypeDescrFloat32)
                , m_name(name)
                , m_member(&member)
                , m_csize(count)
            {
            }

            JsonFieldDescr(const char* name, const char*& member)
                : m_type(JsonType::TypeString)
                , m_typedescr(JsonTypeDescrString)
                , m_name(name)
                , m_member(&member)
                , m_size32(nullptr)
            {
                // const char*  m_name;
            }

            JsonFieldDescr(const char* name, const char**& member, s32& count)
                : m_type(JsonType::TypeString | JsonType::TypeArrayPtr | JsonType::TypeSize32)
                , m_typedescr(JsonTypeDescrString)
                , m_name(name)
                , m_member(&member)
                , m_size32(&count)
            {
                // s32            m_nb_names;
                // const char**   m_names;
            }

            template <typename T>
            JsonFieldDescr(const char* name, T& member, JsonTypeDescr& typeDescr)
                : m_type(JsonType::TypeObject)
                , m_typedescr(&typeDescr)
                , m_name(name)
                , m_member(&member)
                , m_size32(nullptr)
            {
                // e.g. key_t m_key;
            }

            template <typename T>
            JsonFieldDescr(const char* name, T*& member, JsonTypeDescr& typeDescr)
                : m_type(JsonType::TypeObject | JsonType::TypePointer)
                , m_typedescr(&typeDescr)
                , m_name(name)
                , m_member(&member)
                , m_size32(nullptr)
            {
                // e.g. key_t* m_key;
            }

            template <typename T>
            JsonFieldDescr(const char* name, T*& member, s16& count, JsonTypeDescr& typeDescr)
                : m_type(JsonType::TypeObject | JsonType::TypeArrayPtr | JsonType::TypeSize16)
                , m_typedescr(&typeDescr)
                , m_name(name)
                , m_member(&member)
                , m_size16(&count)
            {
                // e.g.
                //  - key_t* m_keys; // array of keys (size<32768)
            }

            template <typename T>
            JsonFieldDescr(const char* name, T*& member, s32& count, JsonTypeDescr& typeDescr)
                : m_type(JsonType::TypeObject | JsonType::TypeArrayPtr | JsonType::TypeSize32)
                , m_typedescr(&typeDescr)
                , m_name(name)
                , m_member(&member)
                , m_size32(&count)
            {
                // e.g.
                //  - key_t* m_keys; // array of keys (size<2147483647)
            }
        };

        struct JsonObject;
        struct JsonNumber;

        struct JsonMember
        {
            inline JsonMember()
                : m_descr(nullptr)
                , m_data_ptr(nullptr)
            {
            }
            inline JsonMember(JsonFieldDescr const* m)
                : m_descr(m)
                , m_data_ptr(nullptr)
            {
            }
            JsonFieldDescr const* m_descr;
            void*                 m_data_ptr;

            inline bool has_descr() const { return m_descr != nullptr; }

            inline bool  is_bool() const { return (m_descr->m_type & JsonType::TypeBool) == JsonType::TypeBool; }
            inline bool  is_int() const { return (m_descr->m_type & (JsonType::TypeInt8 | JsonType::TypeInt16 | JsonType::TypeInt32 | JsonType::TypeInt64)) != 0; }
            inline bool  is_uint() const { return (m_descr->m_type & (JsonType::TypeUInt8 | JsonType::TypeUInt16 | JsonType::TypeUInt32 | JsonType::TypeUInt64)) != 0; }
            inline bool  is_int8() const { return (m_descr->m_type & JsonType::TypeInt8) == JsonType::TypeInt8; }
            inline bool  is_int16() const { return (m_descr->m_type & JsonType::TypeInt16) == JsonType::TypeInt16; }
            inline bool  is_int32() const { return (m_descr->m_type & JsonType::TypeInt32) == JsonType::TypeInt32; }
            inline bool  is_int64() const { return (m_descr->m_type & JsonType::TypeInt64) == JsonType::TypeInt64; }
            inline bool  is_uint8() const { return (m_descr->m_type & JsonType::TypeUInt8) == JsonType::TypeUInt8; }
            inline bool  is_uint16() const { return (m_descr->m_type & JsonType::TypeUInt16) == JsonType::TypeUInt16; }
            inline bool  is_uint32() const { return (m_descr->m_type & JsonType::TypeUInt32) == JsonType::TypeUInt32; }
            inline bool  is_uint64() const { return (m_descr->m_type & JsonType::TypeUInt64) == JsonType::TypeUInt64; }
            inline u32   all_int() const { return (m_descr->m_type & (JsonType::TypeInt8 | JsonType::TypeInt16 | JsonType::TypeInt32 | JsonType::TypeInt64)); }
            inline bool  is_unsigned_integer() const { return (m_descr->m_type & (JsonType::TypeUInt8 | JsonType::TypeUInt16 | JsonType::TypeUInt32 | JsonType::TypeUInt64)) != 0; }
            inline bool  is_signed_integer() const { return (m_descr->m_type & (JsonType::TypeInt8 | JsonType::TypeInt16 | JsonType::TypeInt32 | JsonType::TypeInt64)) != 0; }
            inline bool  is_integer() const { return is_unsigned_integer() || is_signed_integer(); }
            inline bool  is_f32() const { return (m_descr->m_type & JsonType::TypeF32) == JsonType::TypeF32; }
            inline bool  is_f64() const { return (m_descr->m_type & JsonType::TypeF64) == JsonType::TypeF64; }
            inline bool  is_float() const { return (m_descr->m_type & (JsonType::TypeF32 | JsonType::TypeF64)) != 0; }
            inline bool  is_number() const { return (m_descr->m_type & JsonType::TypeNumber) != 0; }
            inline bool  is_string() const { return (m_descr->m_type & JsonType::TypeString) == JsonType::TypeString; }
            inline bool  is_enum() const { return (m_descr->m_type & JsonType::TypeEnum) != 0; }
            inline bool  is_enum16() const { return (m_descr->m_type & JsonType::TypeEnum) == JsonType::TypeEnum16; }
            inline bool  is_enum32() const { return (m_descr->m_type & JsonType::TypeEnum) == JsonType::TypeEnum32; }
            inline bool  is_enum64() const { return (m_descr->m_type & JsonType::TypeEnum) == JsonType::TypeEnum64; }
            inline bool  is_object() const { return (m_descr->m_type & JsonType::TypeObject) == JsonType::TypeObject; }
            inline bool  is_pointer() const { return (m_descr->m_type & JsonType::TypePointer) == JsonType::TypePointer; }
            inline void* get_pointer() const { return *((void**)m_data_ptr); }
            inline bool  is_array() const { return (m_descr->m_type & JsonType::TypeArray) == JsonType::TypeArray; }
            inline bool  is_array_ptr() const { return (m_descr->m_type & JsonType::TypeArrayPtr) == JsonType::TypeArrayPtr; }
            inline bool  is_array_ptr_size8() const { return (m_descr->m_type & (JsonType::TypeArrayPtr | JsonType::TypeSize8)) == (JsonType::TypeArrayPtr | JsonType::TypeSize8); }
            inline bool  is_array_ptr_size16() const { return (m_descr->m_type & (JsonType::TypeArrayPtr | JsonType::TypeSize16)) == (JsonType::TypeArrayPtr | JsonType::TypeSize16); }
            inline bool  is_array_ptr_size32() const { return (m_descr->m_type & (JsonType::TypeArrayPtr | JsonType::TypeSize32)) == (JsonType::TypeArrayPtr | JsonType::TypeSize32); }

            JsonObject get_object(JsonObject const&, JsonAllocator*);
            void*      get_member_ptr(JsonObject const&);
            void*      get_value_ptr(JsonObject const&);
            void       set_string(JsonObject const&, const char* str, const char* str_end);
            void       set_enum(JsonObject const&, const char* str, const char* str_end);
            void       set_number(JsonObject const&, JsonAllocator*, JsonNumber const&);
            void       set_bool(JsonObject const&, JsonAllocator*, bool);
        };

        struct JsonObject
        {
            inline JsonObject()
                : m_descr(nullptr)
                , m_instance(nullptr)
            {
            }
            JsonTypeDescr const* m_descr;
            void*                m_instance;
            JsonMember           get_member(const char* name, const char* name_end) const;
        };

        bool JsonDecode(char const* json, char const* json_end, JsonObject& json_root, JsonAllocator* allocator, JsonAllocator* scratch, char const*& error_message);

    } // namespace njson
} // namespace ncore

#endif // __CJSON_JSON_DECODE_H__

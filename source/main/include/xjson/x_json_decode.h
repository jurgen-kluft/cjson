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
        typedef void (*JsonCopyFn)(void* dst, s32 dst_i, void* src);

        struct JsonTypeDescr;

        struct JsonTypeFuncs
        {
            JsonAllocatorFn  m_alloc;
            JsonCopyFn       m_copy;
        };

        extern JsonTypeFuncs const* JsonFuncsBool;
        extern JsonTypeFuncs const* JsonFuncsInt8;
        extern JsonTypeFuncs const* JsonFuncsInt16;
        extern JsonTypeFuncs const* JsonFuncsInt32;
        extern JsonTypeFuncs const* JsonFuncsInt64;
        extern JsonTypeFuncs const* JsonFuncsFloat32;
        extern JsonTypeFuncs const* JsonFuncsFloat64;
        extern JsonTypeFuncs const* JsonFuncsString;

        struct JsonType
        {
            enum
            {
                TypeNull    = 0x00000,
                TypeBool    = 0x00001,
                TypeInt8    = 0x00002,
                TypeInt16   = 0x00004,
                TypeInt32   = 0x00008,
                TypeInt64   = 0x00010,
                TypeUInt8   = 0x00020,
                TypeUInt16  = 0x00040,
                TypeUInt32  = 0x00080,
                TypeUInt64  = 0x00100,
                TypeF32     = 0x00200,
                TypeF64     = 0x00400,
                TypeNumber  = TypeInt8 | TypeInt16 | TypeInt32 | TypeInt64 | TypeUInt8 | TypeUInt16 | TypeUInt32 | TypeUInt64 | TypeF32 | TypeF64,
                TypeString  = 0x00800,
                TypeObject  = 0x01000,
                TypePointer = 0x02000,
                TypeVector  = 0x04000,
                TypeCarray  = 0x08000,
                TypeSize8   = 0x10000,
                TypeSize16  = 0x20000,
                TypeSize32  = 0x40000,
            };
        };

		struct JsonFieldDescr;

		// Defines a JSON type
		struct JsonTypeDescr
		{
			const char*     m_name;
			void*           m_default;
			int             m_member_count;
			JsonFieldDescr* m_members; // Sorted by name
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

        struct JsonFieldDescr
        {   
            u32                   m_type;
            JsonTypeFuncs const*  m_funcs;
            JsonTypeDescr const*  m_typedescr;
            const char*           m_name;
            void*                 m_member;

            union
            {
                xcore::s32  m_csize;
                xcore::s8*  m_size8;
                xcore::s16* m_size16;
                xcore::s32* m_size32;
            };

            JsonFieldDescr(const char* name, bool& member)
                : m_type(JsonType::TypeBool)
                , m_funcs(JsonFuncsBool)
                , m_typedescr(JsonTypeDescrBool)
                , m_size32(nullptr)
                , m_name(name)
                , m_member(&member)
            {
            }
            JsonFieldDescr(const char* name, bool*& member)
                : m_type(JsonType::TypeBool | JsonType::TypePointer)
                , m_funcs(JsonFuncsBool)
                , m_typedescr(JsonTypeDescrBool)
                , m_size32(nullptr)
                , m_name(name)
                , m_member(&member)
            {
            }

            JsonFieldDescr(const char* name, bool*& member, s32 count)
                : m_type(JsonType::TypeBool | JsonType::TypeCarray)
                , m_funcs(JsonFuncsBool)
                , m_typedescr(JsonTypeDescrBool)
                , m_csize(count)
                , m_name(name)
                , m_member(&member)
            {
            }

            JsonFieldDescr(const char* name, bool*& member, s32& count)
                : m_type(JsonType::TypeBool | JsonType::TypeVector | JsonType::TypeSize32)
                , m_funcs(JsonFuncsBool)
                , m_typedescr(JsonTypeDescrBool)
                , m_size32(&count)
                , m_name(name)
                , m_member(&member)
            {
            }

            JsonFieldDescr(const char* name, xcore::s8& member)
                : m_type(JsonType::TypeInt8)
                , m_funcs(JsonFuncsInt8)
                , m_typedescr(JsonTypeDescrInt8)
                , m_size32(nullptr)
                , m_name(name)
                , m_member(&member)
            {
            }

            JsonFieldDescr(const char* name, xcore::s8*& member)
                : m_type(JsonType::TypeInt8 | JsonType::TypePointer)
                , m_funcs(JsonFuncsInt8)
                , m_typedescr(JsonTypeDescrInt8)
                , m_size32(nullptr)
                , m_name(name)
                , m_member(&member)
            {
            }

            JsonFieldDescr(const char* name, xcore::s8*& member, s32 count)
                : m_type(JsonType::TypeInt8 | JsonType::TypeCarray)
                , m_funcs(JsonFuncsInt8)
                , m_typedescr(JsonTypeDescrInt8)
                , m_csize(count)
                , m_name(name)
                , m_member(&member)
            {
            }

            JsonFieldDescr(const char* name, xcore::s8*& member, s32& count)
                : m_type(JsonType::TypeInt8 | JsonType::TypeVector | JsonType::TypeSize8)
                , m_funcs(JsonFuncsInt8)
                , m_typedescr(JsonTypeDescrInt8)
                , m_size32(&count)
                , m_name(name)
                , m_member(&member)
            {
            }

            JsonFieldDescr(const char* name, xcore::s16& member)
                : m_type(JsonType::TypeInt16) 
                , m_funcs(JsonFuncsInt16)
                , m_typedescr(JsonTypeDescrInt16)
                , m_size32(nullptr)
                , m_name(name)
                , m_member(&member)
            {
            }

            JsonFieldDescr(const char* name, xcore::s16*& member)
                : m_type(JsonType::TypeInt16 | JsonType::TypePointer)
                , m_funcs(JsonFuncsInt16)
                , m_typedescr(JsonTypeDescrInt16)
                , m_size32(nullptr)
                , m_name(name)
                , m_member(&member)
            {
            }

            JsonFieldDescr(const char* name, xcore::s16*& member, s32 count)
                : m_type(JsonType::TypeInt16 | JsonType::TypeCarray)
                , m_funcs(JsonFuncsInt16)
                , m_typedescr(JsonTypeDescrInt16)
                , m_csize(count)
                , m_name(name)
                , m_member(&member)
            {
            }

            JsonFieldDescr(const char* name, xcore::s16*& member, s32& count)
                : m_type(JsonType::TypeInt16 | JsonType::TypeVector | JsonType::TypeSize16)
                , m_funcs(JsonFuncsInt16)
                , m_typedescr(JsonTypeDescrInt16)
                , m_size32(&count)
                , m_name(name)
                , m_member(&member)
            {
            }

            JsonFieldDescr(const char* name, xcore::s32& member)
                : m_type(JsonType::TypeInt32)
                , m_funcs(JsonFuncsInt32)
                , m_typedescr(JsonTypeDescrInt32)
                , m_size32(nullptr)
                , m_name(name)
                , m_member(&member)
            {
            }

            JsonFieldDescr(const char* name, xcore::s32*& member)
                : m_type(JsonType::TypeInt32 | JsonType::TypePointer)
                , m_funcs(JsonFuncsInt32)
                , m_typedescr(JsonTypeDescrInt32)
                , m_size32(nullptr)
                , m_name(name)
                , m_member(&member)
            {
            }

            JsonFieldDescr(const char* name, xcore::s32*& member, s32 count)
                : m_type(JsonType::TypeInt32 | JsonType::TypeCarray)
                , m_funcs(JsonFuncsInt32)
                , m_typedescr(JsonTypeDescrInt32)
                , m_csize(count)
                , m_name(name)
                , m_member(&member)
            {
            }

            JsonFieldDescr(const char* name, xcore::s32*& member, s32& count)
                : m_type(JsonType::TypeInt32 | JsonType::TypeVector | JsonType::TypeSize32)
                , m_funcs(JsonFuncsInt32)
                , m_typedescr(JsonTypeDescrInt32)
                , m_size32(&count)
                , m_name(name)
                , m_member(&member)
            {
            }

            JsonFieldDescr(const char* name, float& member)
                : m_type(JsonType::TypeF32)
                , m_funcs(JsonFuncsFloat32)
                , m_typedescr(JsonTypeDescrFloat32)
                , m_size32(nullptr)
                , m_name(name)
                , m_member(&member)
            {
            }

            template <s32 N>
            JsonFieldDescr(const char* name, float (&member)[N], s32 count)
                : m_type(JsonType::TypeF32 | JsonType::TypeCarray)
                , m_funcs(JsonFuncsFloat32)
                , m_typedescr(JsonTypeDescrFloat32)
                , m_csize(count)
                , m_name(name)
                , m_member(&member)
            {
            }

            JsonFieldDescr(const char* name, float*& member)
                : m_type(JsonType::TypeF32 | JsonType::TypePointer)
                , m_funcs(JsonFuncsFloat32)
                , m_typedescr(JsonTypeDescrFloat32)
                , m_size32(nullptr)
                , m_name(name)
                , m_member(&member)
            {
            }

            JsonFieldDescr(const char* name, float*& member, s32 count)
                : m_type(JsonType::TypeF32 | JsonType::TypeCarray)
                , m_funcs(JsonFuncsFloat32)
                , m_typedescr(JsonTypeDescrFloat32)
                , m_csize(count)
                , m_name(name)
                , m_member(&member)
            {
            }

            JsonFieldDescr(const char* name, float*& member, s8& count)
                : m_type(JsonType::TypeF32 | JsonType::TypeVector | JsonType::TypeSize8)
                , m_funcs(JsonFuncsFloat32)
                , m_typedescr(JsonTypeDescrFloat32)
                , m_size8(&count)
                , m_name(name)
                , m_member(&member)
            {
            }

            JsonFieldDescr(const char* name, float*& member, s16& count)
                : m_type(JsonType::TypeF32 | JsonType::TypeVector | JsonType::TypeSize16)
                , m_funcs(JsonFuncsFloat32)
                , m_typedescr(JsonTypeDescrFloat32)
                , m_size16(&count)
                , m_name(name)
                , m_member(&member)
            {
            }

            JsonFieldDescr(const char* name, float*& member, s32& count)
                : m_type(JsonType::TypeF32 | JsonType::TypeVector | JsonType::TypeSize32)
                , m_funcs(JsonFuncsFloat32)
                , m_typedescr(JsonTypeDescrFloat32)
                , m_size32(&count)
                , m_name(name)
                , m_member(&member)
            {
            }

            template <s32 N>
            JsonFieldDescr(const char* name, float* (&member)[N], s32 count = N)
                : m_type(JsonType::TypeF32 | JsonType::TypeCarray)
                , m_funcs(JsonFuncsFloat32)
                , m_typedescr(JsonTypeDescrFloat32)
                , m_csize(count)
                , m_name(name)
                , m_member(&member)
            {
            }

            JsonFieldDescr(const char* name, const char*& member)
                : m_type(JsonType::TypeString)
                , m_funcs(JsonFuncsString)
                , m_typedescr(JsonTypeDescrString)
                , m_size32(nullptr)
                , m_name(name)
                , m_member(&member)
            {
            }

            template <typename T>
            JsonFieldDescr(const char* name, T& member, JsonTypeFuncs& typeFuncs, JsonTypeDescr& typeDescr)
                : m_type(JsonType::TypeObject)
                , m_funcs(&typeFuncs)
                , m_typedescr(&typeDescr)
                , m_name(name)
                , m_size32(nullptr)
                , m_member(&member)
            {
                // e.g. key_t m_key;
            }

            template <typename T>
            JsonFieldDescr(const char* name, T*& member, JsonTypeFuncs& typeFuncs, JsonTypeDescr& typeDescr)
                : m_type(JsonType::TypeObject | JsonType::TypePointer)
				, m_funcs(&typeFuncs)
				, m_typedescr(&typeDescr)
				, m_name(name)
                , m_size32(nullptr)
                , m_member(&member)
            {
                // e.g. key_t* m_key;
            }

            template <typename T>
            JsonFieldDescr(const char* name, T*& member, xcore::s16& count, JsonTypeFuncs& typeFuncs, JsonTypeDescr& typeDescr)
                : m_type(JsonType::TypeObject | JsonType::TypeVector | JsonType::TypeSize16)
				, m_funcs(&typeFuncs)
				, m_typedescr(&typeDescr)
				, m_name(name)
                , m_size16(&count)
                , m_member(&member)
            {
                // e.g.
                //  - key_t* m_keys; // array of keys (size<32768)
            }

            template <typename T>
            JsonFieldDescr(const char* name, T*& member, xcore::s32& count, JsonTypeFuncs& typeFuncs, JsonTypeDescr& typeDescr)
                : m_type(JsonType::TypeObject | JsonType::TypeVector | JsonType::TypeSize32)
				, m_funcs(&typeFuncs)
				, m_typedescr(&typeDescr)
				, m_name(name)
                , m_size32(&count)
                , m_member(&member)
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
            JsonFieldDescr const*  m_descr;
            void*                  m_data_ptr;

            inline bool is_bool() const { return (m_descr->m_type & JsonType::TypeBool) == JsonType::TypeBool; }
            inline bool is_int8() const { return (m_descr->m_type & JsonType::TypeInt8) == JsonType::TypeInt8; }
            inline bool is_int16() const { return (m_descr->m_type & JsonType::TypeInt16) == JsonType::TypeInt16; }
            inline bool is_int32() const { return (m_descr->m_type & JsonType::TypeInt32) == JsonType::TypeInt32; }
            inline bool is_int64() const { return (m_descr->m_type & JsonType::TypeInt64) == JsonType::TypeInt64; }
            inline bool is_uint8() const { return (m_descr->m_type & JsonType::TypeUInt8) == JsonType::TypeUInt8; }
            inline bool is_uint16() const { return (m_descr->m_type & JsonType::TypeUInt16) == JsonType::TypeUInt16; }
            inline bool is_uint32() const { return (m_descr->m_type & JsonType::TypeUInt32) == JsonType::TypeUInt32; }
            inline bool is_uint64() const { return (m_descr->m_type & JsonType::TypeUInt64) == JsonType::TypeUInt64; }
            inline bool is_f32() const { return (m_descr->m_type & JsonType::TypeF32) == JsonType::TypeF32; }
            inline bool is_f64() const { return (m_descr->m_type & JsonType::TypeF64) == JsonType::TypeF64; }
            inline bool is_number() const { return (m_descr->m_type & JsonType::TypeNumber) != 0; }
            inline bool is_string() const { return (m_descr->m_type & JsonType::TypeString) == JsonType::TypeString; }
            inline bool is_object() const { return (m_descr->m_type & JsonType::TypeObject) == JsonType::TypeObject; }
            inline bool is_pointer() const { return (m_descr->m_type & JsonType::TypePointer) == JsonType::TypePointer; }
            inline bool is_vector() const { return (m_descr->m_type & JsonType::TypeVector) == JsonType::TypeVector; }
            inline bool is_carray() const { return (m_descr->m_type & JsonType::TypeCarray) == JsonType::TypeCarray; }

            JsonObject get_object(JsonObject const&, JsonAllocator*);
            void       set_string(JsonObject const&, const char*);
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
            void*                  m_instance;
            JsonMember             get_member(const char* name) const;
        };

        bool JsonDecode(char const* json, char const* json_end, JsonObject& json_root, JsonAllocator* allocator, JsonAllocator* scratch, char const*& error_message);

    } // namespace json
} // namespace xcore

#endif // __XBASE_JSON_H__
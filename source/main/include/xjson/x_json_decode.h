#ifndef __XBASE_JSON_DECODE_H__
#define __XBASE_JSON_DECODE_H__
#include "xbase/x_target.h"
#ifdef USE_PRAGMA_ONCE
#pragma once
#endif

namespace xcore
{
	enum jsonvalue_type
	{
		jsonvalue_type_null     = 0x0000,
		jsonvalue_type_bool     = 0x0001,
		jsonvalue_type_int8     = 0x0002,
		jsonvalue_type_int16    = 0x0003,
		jsonvalue_type_int32    = 0x0004,
		jsonvalue_type_int64    = 0x0005,
		jsonvalue_type_f32      = 0x0006,
		jsonvalue_type_f64      = 0x0007,
		jsonvalue_type_string   = 0x0008,
		jsonvalue_type_object   = 0x0009,
		jsonvalue_type_pointer  = 0x0200,
		jsonvalue_type_array    = 0x0400,
		jsonvalue_type_size_s8  = 0x0800,
		jsonvalue_type_size_s16 = 0x1000,
	};

	struct jsonobject_t;

	struct jsonvalue_t
	{
		jsonvalue_t(const char* name, bool* value)
			: type(jsonvalue_type_bool)
			, count(nullptr)
			, name(name)
			, value(value)
			, object(nullptr)
		{
		}
		jsonvalue_t(const char* name, bool** value)
			: type(jsonvalue_type_bool | jsonvalue_type_pointer)
			, count(nullptr)
			, name(name)
			, value(value)
			, object(nullptr)
		{
		}

		jsonvalue_t(const char* name, xcore::s8* value)
			: type(jsonvalue_type_int8)
			, count(nullptr)
			, name(name)
			, value(value)
			, object(nullptr)
		{
		}

		jsonvalue_t(const char* name, xcore::s16* value)
			: type(jsonvalue_type_int16)
			, count(nullptr)
			, name(name)
			, value(value)
			, object(nullptr)
		{
		}

		jsonvalue_t(const char* name, xcore::s32* value)
			: type(jsonvalue_type_int32)
			, count(nullptr)
			, name(name)
			, value(value)
			, object(nullptr)
		{
		}

		jsonvalue_t(const char* name, xcore::s64* value)
			: type(jsonvalue_type_int64)
			, count(nullptr)
			, name(name)
			, value(value)
			, object(nullptr)
		{
		}

		jsonvalue_t(const char* name, xcore::f32* value)
			: type(jsonvalue_type_f32)
			, count(nullptr)
			, name(name)
			, value(value)
			, object(nullptr)
		{
		}

		jsonvalue_t(const char* name, xcore::f32** value, xcore::s8* count)
			: type(jsonvalue_type_f32 | jsonvalue_type_size_s8 | jsonvalue_type_array)
			, count8(count)
			, name(name)
			, value(value)
			, object(nullptr)
		{
		}

		jsonvalue_t(const char* name, xcore::f32** value, xcore::s32* count)
			: type(jsonvalue_type_f32 | jsonvalue_type_array)
			, count(count)
			, name(name)
			, value(value)
			, object(nullptr)
		{
		}

		jsonvalue_t(const char* name, xcore::f64* value)
			: type(jsonvalue_type_f64)
			, count(nullptr)
			, name(name)
			, value(value)
			, object(nullptr)
		{
			// e.g. f64 m_width;
		}

		jsonvalue_t(const char* name, xcore::f64** value)
			: type(jsonvalue_type_f64 | jsonvalue_type_pointer)
			, count(nullptr)
			, name(name)
			, value(value)
			, object(nullptr)
		{
			// e.g. f64* m_width;
		}

		jsonvalue_t(const char* name, xcore::f64** value, xcore::s32* count)
			: type(jsonvalue_type_f64 | jsonvalue_type_array)
			, count(count)
			, name(name)
			, value(value)
			, object(nullptr)
		{
			// e.g.
			// f64* m_width;
		}

		jsonvalue_t(const char* name, const char** value)
			: type(jsonvalue_type_string)
			, count(0)
			, name(name)
			, value((void*)value)
			, object(nullptr)
		{
			// e.g. const char* m_label;
		}

		jsonvalue_t(const char* name, void* value, jsonobject_t* obj)
			: type(jsonvalue_type_object)
			, count(0)
			, name(name)
			, value(value)
			, object(nullptr)
		{
			// e.g. key_t m_key;
		}

		jsonvalue_t(const char* name, void** value, jsonobject_t* obj)
			: type(jsonvalue_type_object | jsonvalue_type_pointer)
			, name(name)
			, count(0)
			, value(value)
			, object(nullptr)
		{
			// e.g. key_t* m_key;
		}

		template <typename T>
		jsonvalue_t(const char* _name, T** _value, xcore::s16* _count, jsonobject_t* _obj)
			: type(jsonvalue_type_object | jsonvalue_type_array | jsonvalue_type_size_s16)
			, name(_name)
			, count16(_count)
			, value(_value)
			, object(_obj)
		{
			// e.g.
			//  - key_t* m_keys; // array of keys
		}

		template <typename T>
		jsonvalue_t(const char* _name, T** _value, xcore::s32* _count, jsonobject_t* _obj)
			: type(jsonvalue_type_object | jsonvalue_type_array)
			, name(_name)
			, count(_count)
			, value(_value)
			, object(_obj)
		{
			// e.g.
			//  - key_t* m_keys; // array of keys
		}

		xcore::u32  type;
		const char* name;
		union
		{
			xcore::s8*  count8;
			xcore::s16* count16;
			xcore::s32* count;
		};
		void*         value;
		jsonobject_t* object;
	};

	class json_allocator_t
	{
	public:
		template <typename T> inline T* new_instance() { return (T*)allocate(sizeof(T), ALIGNOF(T)); }
		template <typename T> inline T* new_array(u32 size) { return (T*)allocate(sizeof(T) * size, ALIGNOF(T)); }

		void* scope_begin();
		void  scope_end(void*);

		void* checkout();
		void  commmit(void*);

	protected:
		void* allocate(xcore::u32 size, xcore::u32 alignment);
	};

	// typedef function that returns a pointer to a new instance of an object
	typedef void* (*json_construct_fn)(json_allocator_t* alloc);

	struct jsonobject_t
	{
		const char*       m_name;
		void*             m_default;
		json_construct_fn m_constructor;
		int               m_member_count;
		jsonvalue_t*      m_members; // Sorted by name
	};

	namespace json
	{
		struct MemAllocLinear;
	}

	bool json_decode(char const* json, char const* json_end, jsonobject_t* json_root, void* root, json::MemAllocLinear* allocator, json::MemAllocLinear* scratch, char const*& error_message);

} // namespace xcore

#endif // __XBASE_JSON_H__
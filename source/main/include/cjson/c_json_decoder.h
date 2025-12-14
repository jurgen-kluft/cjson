#ifndef __CJSON_JSON_DECODER_H__
#define __CJSON_JSON_DECODER_H__
#include "ccore/c_target.h"
#ifdef USE_PRAGMA_ONCE
#    pragma once
#endif

namespace ncore
{
    namespace njson
    {
        struct JsonAllocator;

        namespace ndecoder
        {
            // TODO: replace with 'typedef u32 result_t', and define
            // #define RESULT_VALID(r)      ((r) == 3)
            // #define RESULT_OK(r)         (((r)&1) == 1)
            // #define RESULT_NOT_OK(r)     (((r)&1) == 0)
            // #define RESULT_END(r)        (((r)&2) == 2)
            // #define RESULT_NOT_END(r)    (((r)&2) == 0)

            struct result_t
            {
                inline result_t()
                    : m_result(0)
                {
                }
                inline result_t(bool ok, bool end)
                    : m_result((ok ? 1 : 0) | (end ? 2 : 0))
                {
                }

                inline bool ok() const { return (m_result & 1) == 1; }
                inline bool not_ok() const { return (m_result & 1) == 0; }
                inline bool end() const { return (m_result & 2) == 2; }
                inline bool not_end() const { return (m_result & 2) == 0; }
                inline bool valid() const { return (m_result & 3) == 1; } // valid means ok and not end

            private:
                u32 m_result;
            };

            struct state_t;
            struct state_array_t;

            struct decoder_t
            {
                JsonAllocator* m_DecoderAllocator;
                JsonAllocator* m_StackAllocator;
                state_t*       m_CurrentState;
            };

            decoder_t* create_decoder(JsonAllocator* scratch_allocator, JsonAllocator* decoder_allocator, const char* json, const char* json_end);
            void       destroy_decoder(decoder_t*& d);

            void decode_bool(decoder_t* d, bool& out_value);
            void decode_i16(decoder_t* d, i16& out_value);
            void decode_i32(decoder_t* d, i32& out_value);
            void decode_f32(decoder_t* d, f32& out_value);
            void decode_string(decoder_t* d, const char*& out_str);            // Unbounded string
            void decode_cstring(decoder_t* d, char* out_str, i32 out_str_len); // Bounded C string

            void decode_array_bool(decoder_t* d, bool*& out_array, i32& out_array_size, i32 out_array_maxsize);
            void decode_array_u8(decoder_t* d, u8*& out_array, i32& out_array_size, i32 out_array_maxsize);
            void decode_array_u16(decoder_t* d, u16*& out_array, i32& out_array_size, i32 out_array_maxsize);
            void decode_array_u32(decoder_t* d, u32*& out_array, i32& out_array_size, i32 out_array_maxsize);
            void decode_array_u64(decoder_t* d, u64*& out_array, i32& out_array_size, i32 out_array_maxsize);
            void decode_array_i8(decoder_t* d, i8*& out_array, i32& out_array_size, i32 out_array_maxsize);
            void decode_array_i16(decoder_t* d, i16*& out_array, i32& out_array_size, i32 out_array_maxsize);
            void decode_array_i32(decoder_t* d, i32*& out_array, i32& out_array_size, i32 out_array_maxsize);
            void decode_array_i64(decoder_t* d, i64*& out_array, i32& out_array_size, i32 out_array_maxsize);
            void decode_array_char(decoder_t* d, char*& out_array, i32& out_array_size, i32 out_array_maxsize);
            void decode_array_f32(decoder_t* d, f32*& out_array, i32& out_array_size, i32 out_array_maxsize);

            void decode_carray_bool(decoder_t* d, bool* out_array, i32 out_array_maxlen);
            void decode_carray_u8(decoder_t* d, u8* out_array, i32 out_array_maxlen);
            void decode_carray_u16(decoder_t* d, u16* out_array, i32 out_array_maxlen);
            void decode_carray_u32(decoder_t* d, u32* out_array, i32 out_array_maxlen);
            void decode_carray_u64(decoder_t* d, u64* out_array, i32 out_array_maxlen);
            void decode_carray_i8(decoder_t* d, i8* out_array, i32 out_array_maxlen);
            void decode_carray_i16(decoder_t* d, i16* out_array, i32 out_array_maxlen);
            void decode_carray_i32(decoder_t* d, i32* out_array, i32 out_array_maxlen);
            void decode_carray_i64(decoder_t* d, i64* out_array, i32 out_array_maxlen);
            void decode_carray_char(decoder_t* d, char* out_array, i32 out_array_maxlen);
            void decode_carray_f32(decoder_t* d, f32* out_array, i32 out_array_maxlen);

            void decode_enum(decoder_t* d, u8& out_enum_value, const char** enum_strs, const u8* enum_values, i32 enum_count, bool as_flags);
            void decode_enum(decoder_t* d, u16& out_enum_value, const char** enum_strs, const u16* enum_values, i32 enum_count, bool as_flags);
            void decode_enum(decoder_t* d, u32& out_enum_value, const char** enum_strs, const u32* enum_values, i32 enum_count, bool as_flags);
            void decode_enum(decoder_t* d, u64& out_enum_value, const char** enum_strs, const u64* enum_values, i32 enum_count, bool as_flags);

            // void decoder_begin_members(decoder_t* d, i32 capacity);
            void decoder_add_member(decoder_t* d, const char* name, bool* out_value, i32 out_value_len = 1);
            void decoder_add_member(decoder_t* d, const char* name, u8* out_value, i32 out_value_len = 1);
            void decoder_add_member(decoder_t* d, const char* name, u16* out_value, i32 out_value_len = 1);
            void decoder_add_member(decoder_t* d, const char* name, u32* out_value, i32 out_value_len = 1);
            void decoder_add_member(decoder_t* d, const char* name, u64* out_value, i32 out_value_len = 1);
            void decoder_add_member(decoder_t* d, const char* name, i8* out_value, i32 out_value_len = 1);
            void decoder_add_member(decoder_t* d, const char* name, i16* out_value, i32 out_value_len = 1);
            void decoder_add_member(decoder_t* d, const char* name, i32* out_value, i32 out_value_len = 1);
            void decoder_add_member(decoder_t* d, const char* name, i64* out_value, i32 out_value_len = 1);
            void decoder_add_member(decoder_t* d, const char* name, f32* out_value, i32 out_value_len = 1);
            void decoder_add_member(decoder_t* d, const char* name, char* out_value, i32 out_value_len = 1);
            void decoder_add_member(decoder_t* d, const char* name, const char** out_value, i32 out_value_len = 1);

            void decoder_add_member(decoder_t* d, const char* name, u8** out_value, i32* out_len, i32 max_len = -1);
            void decoder_add_member(decoder_t* d, const char* name, u16** out_value, i32* out_len, i32 max_len = -1);
            void decoder_add_member(decoder_t* d, const char* name, u32** out_value, i32* out_len, i32 max_len = -1);
            void decoder_add_member(decoder_t* d, const char* name, u64** out_value, i32* out_len, i32 max_len = -1);
            void decoder_add_member(decoder_t* d, const char* name, i8** out_value, i32* out_len, i32 max_len = -1);
            void decoder_add_member(decoder_t* d, const char* name, i16** out_value, i32* out_len, i32 max_len = -1);
            void decoder_add_member(decoder_t* d, const char* name, i32** out_value, i32* out_len, i32 max_len = -1);
            void decoder_add_member(decoder_t* d, const char* name, i64** out_value, i32* out_len, i32 max_len = -1);
            void decoder_add_member(decoder_t* d, const char* name, f32** out_value, i32* out_len, i32 max_len = -1);
            void decoder_add_member(decoder_t* d, const char* name, f32** out_value, i8* out_len, i32 max_len = -1);
            void decoder_add_member(decoder_t* d, const char* name, char** out_value, i32* out_len, i32 max_len = -1);

            void decoder_add_enum_member(decoder_t* d, const char* name, const char** enum_strs, const void* enum_values, i8 sizeof_enum_value, i32 enum_count, bool as_flags, void* out_value, i8 sizeof_out_value);

            template <typename T> void decoder_add_enum_member(decoder_t* d, const char* name, const char** enum_strs, const T* enum_values, i32 enum_count, T* out_value)
            {
                decoder_add_enum_member(d, name, enum_strs, enum_values, sizeof(T), enum_count, false, out_value, sizeof(T));
            }

            template <typename T> void decoder_add_flag_member(decoder_t* d, const char* name, const char** flag_strs, const T* flag_values, i32 flag_count, T* out_value)
            {
                decoder_add_enum_member(d, name, flag_strs, flag_values, sizeof(T), flag_count, true, out_value, sizeof(T));
            }

            result_t read_object_begin(decoder_t* d);
            result_t read_object_end(decoder_t* d);
            result_t read_array_begin(decoder_t* d, i32& out_size);
            result_t read_array_end(decoder_t* d);

            struct field_t
            {
                const char* m_Name;
                const char* m_End;

                inline bool is_valid() const { return m_Name != nullptr && m_End != nullptr; }
            };
            field_t decode_field(decoder_t* d);

            inline bool field_equal(const field_t& field, const char* cmp_name)
            {
                ASSERT(field.is_valid());
                const char* fptr = field.m_Name;
                const char* cptr = cmp_name;
                while (fptr < field.m_End && *cptr != 0 && *fptr == *cptr)
                {
                    ++fptr;
                    ++cptr;
                }
                return (fptr == field.m_End) && (*cptr == 0);
            }

            // @return: true if member was found and decoded, false otherwise
            bool decoder_decode_member(decoder_t* d, field_t const& field);

            // Accept a lambda for decoding object
            template <typename TDecodeFn> void decode_object(decoder_t* d, const TDecodeFn* decode_fn)
            {
                result_t result = read_object_begin(d);
                while (result.valid())
                {
                    field_t field = decode_field(d);
                    decode_fn(d, field);
                    result = read_object_end(d);
                }
            }

            inline bool field_set_i32(decoder_t* d, const field_t& field, const char* name, i32& out_value)
            {
                if (field_equal(field, name))
                {
                    decode_i32(d, out_value);
                    return true;
                }
                return false;
            }

            inline bool field_set_f32(decoder_t* d, const field_t& field, const char* name, f32& out_value)
            {
                if (field_equal(field, name))
                {
                    decode_f32(d, out_value);
                    return true;
                }
                return false;
            }

            inline bool field_set_cstring(decoder_t* d, const field_t& field, const char* name, char* out_str, u32 out_str_max_len)
            {
                if (field_equal(field, name))
                {
                    decode_cstring(d, out_str, out_str_max_len);
                    return true;
                }
                return false;
            }

        } // namespace ndecoder
    } // namespace njson
} // namespace ncore

#endif // __CJSON_JSON_DECODER_H__

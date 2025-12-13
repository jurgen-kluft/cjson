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
            struct result_t
            {
                inline result_t()
                    : ok(false)
                    , end(false)
                {
                }
                inline result_t(bool ok, bool end)
                    : ok(ok)
                    , end(end)
                {
                }
                bool ok;
                bool end;
            };

            struct state_t;
            struct state_array_t;

            struct decoder_t
            {
                JsonAllocator* m_ScratchAllocator;
                JsonAllocator* m_DecoderAllocator;
                state_array_t* m_ReusableStates;
                state_array_t* m_Stack;
                state_t*       m_Current;
            };

            decoder_t* create_decoder(JsonAllocator* scratch_allocator, JsonAllocator* decoder_allocator, const char* json, const char* json_end);
            void       destroy_decoder(decoder_t*& d);

            void decode_bool(decoder_t* d, bool& out_value);
            void decode_i16(decoder_t* d, i16& out_value);
            void decode_i32(decoder_t* d, i32& out_value);
            void decode_f32(decoder_t* d, f32& out_value);
            void decode_string(decoder_t* d, const char*& out_str);            // Unbounded string
            void decode_cstring(decoder_t* d, char* out_str, i32 out_str_len); // Bounded C string
            void decode_array_f32(decoder_t* d, f32*& out_array, i32& out_array_size, i32 out_array_maxsize);
            void decode_carray_f32(decoder_t* d, f32* out_array, i32 out_array_maxlen);

            void decode_enum(decoder_t* d, u64& out_enum_value, const char** enum_strs, const u64* enum_values, i32 enum_count);

            result_t read_until_object_end(decoder_t* d);
            result_t read_until_array_end(decoder_t* d, i32& out_size);
            result_t read_until_array_end(decoder_t* d);

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

            // Accept a lambda for decoding object
            template <typename TDecodeFn> void decode_object(decoder_t* d, const TDecodeFn* decode_fn)
            {
                result_t result = read_until_object_end(d);
                while (result.ok && !result.end)
                {
                    field_t field = decode_field(d);
                    decode_fn(d, field);
                    result = read_until_object_end(d);
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

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
            typedef u32 result_t;
            inline bool NotOk(result_t r) { return (((r) & 1) == 0); }
            inline bool Ended(result_t r) { return (((r) & 2) == 2); }
            inline bool OkAndEnded(result_t r) { return (((r) & 3) == 3); }
            inline bool OkAndNotEnded(result_t r) { return (((r) & 3) == 1); }

            const result_t ResultInvalid       = 0;
            const result_t ResultOk            = 1;
            const result_t ResultEnded         = 2;
            const result_t ResultOkAndNotEnded = 1;
            const result_t ResultOkAndEnded    = 3;

            struct state_t;
            struct state_array_t;

            struct decoder_t
            {
                JsonAllocator* m_DecoderAllocator;
                JsonAllocator* m_StackAllocator;
                s64            m_StackAllocatorInitialSize;
                state_t*       m_CurrentState;
            };

            decoder_t* create_decoder(JsonAllocator* scratch_allocator, JsonAllocator* decoder_allocator, const char* json, const char* json_end);
            void       destroy_decoder(decoder_t*& d);

            enum EType
            {
                TYPE_INVALID = 0,
                TYPE_BOOL,
                TYPE_I8,
                TYPE_I16,
                TYPE_I32,
                TYPE_I64,
                TYPE_U8,
                TYPE_U16,
                TYPE_U32,
                TYPE_U64,
                TYPE_F32,
                TYPE_CHAR,
                TYPE_STRING,
                TYPE_MAC_ADDR,
                // Array types
                TYPE_BOOL_ARRAY,
                TYPE_I8_ARRAY,
                TYPE_I16_ARRAY,
                TYPE_I32_ARRAY,
                TYPE_I64_ARRAY,
                TYPE_U8_ARRAY,
                TYPE_U16_ARRAY,
                TYPE_U32_ARRAY,
                TYPE_U64_ARRAY,
                TYPE_F32_ARRAY,
                TYPE_CHAR_ARRAY,
                TYPE_STRING_ARRAY
            };

            bool decode_bool(decoder_t* d);
            i8   decode_i8(decoder_t* d);
            i16  decode_i16(decoder_t* d);
            i32  decode_i32(decoder_t* d);
            i64  decode_i64(decoder_t* d);
            u8   decode_u8(decoder_t* d);
            u16  decode_u16(decoder_t* d);
            u32  decode_u32(decoder_t* d);
            u64  decode_u64(decoder_t* d);

            f32         decode_f32(decoder_t* d);
            const char* decode_string(decoder_t* d);                                  // Unbounded string
            void        decode_cstring(decoder_t* d, char* out_str, i32 out_str_len); // Bounded C string

            void decode_array_bool(decoder_t* d, bool*& out_array, i32& out_array_size, i32 out_array_maxsize);
            void decode_array_u8(decoder_t* d, u8*& out_array, i32& out_array_size, i32 out_array_maxsize);
            void decode_array_u16(decoder_t* d, u16*& out_array, i32& out_array_size, i32 out_array_maxsize);
            void decode_array_u32(decoder_t* d, u32*& out_array, i32& out_array_size, i32 out_array_maxsize);
            void decode_array_u64(decoder_t* d, u64*& out_array, i32& out_array_size, i32 out_array_maxsize);
            void decode_array_i8(decoder_t* d, i8*& out_array, i32& out_array_size, i32 out_array_maxsize);
            void decode_array_i16(decoder_t* d, i16*& out_array, i32& out_array_size, i32 out_array_maxsize);
            void decode_array_i32(decoder_t* d, i32*& out_array, i32& out_array_size, i32 out_array_maxsize);
            void decode_array_i64(decoder_t* d, i64*& out_array, i32& out_array_size, i32 out_array_maxsize);
            void decode_array_f32(decoder_t* d, f32*& out_array, i32& out_array_size, i32 out_array_maxsize);
            void decode_array_char(decoder_t* d, char*& out_array, i32& out_array_size, i32 out_array_maxsize);
            void decode_array_str(decoder_t* d, const char**& out_array, i32& out_array_size, i32 out_array_maxsize);

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
            void decode_carray_str(decoder_t* d, const char** out_array, i32 out_array_maxlen);
            void decode_carray_f32(decoder_t* d, f32* out_array, i32 out_array_maxlen);

            // ----------------------------------------------------------------------------------------------------------------------------------------
            // For Json object decoding
            struct system_type_t
            {
                system_type_t(bool* out_value);
                system_type_t(u8* out_value);
                system_type_t(u16* out_value);
                system_type_t(u32* out_value);
                system_type_t(u64* out_value);
                system_type_t(i8* out_value);
                system_type_t(i16* out_value);
                system_type_t(i32* out_value);
                system_type_t(i64* out_value);
                system_type_t(f32* out_value);
                system_type_t(char* out_value);
                system_type_t(const char** out_value);
                union
                {
                    void*        m_void;
                    bool*        m_bool;
                    u8*          m_u8;
                    u16*         m_u16;
                    u32*         m_u32;
                    u64*         m_u64;
                    i8*          m_i8;
                    i16*         m_i16;
                    i32*         m_i32;
                    i64*         m_i64;
                    f32*         m_f32;
                    char*        m_char;
                    const char** m_str;
                };
                u8 m_type;
                u8 m_value_type;
            };

            void register_member(decoder_t* d, const char* name, system_type_t type);

            // Custom types
            void register_mac_addr(decoder_t* d, const char* name, system_type_t type);

            struct carray_type_t
            {
                carray_type_t(bool* out_value, i32 out_value_len);
                carray_type_t(u8* out_value, i32 out_value_len);
                carray_type_t(u16* out_value, i32 out_value_len);
                carray_type_t(u32* out_value, i32 out_value_len);
                carray_type_t(u64* out_value, i32 out_value_len);
                carray_type_t(i8* out_value, i32 out_value_len);
                carray_type_t(i16* out_value, i32 out_value_len);
                carray_type_t(i32* out_value, i32 out_value_len);
                carray_type_t(i64* out_value, i32 out_value_len);
                carray_type_t(f32* out_value, i32 out_value_len);
                carray_type_t(char* out_value, i32 out_value_len);
                carray_type_t(const char** out_value, i32 out_value_len);
                union
                {
                    void*        m_void;
                    bool*        m_bool;
                    u8*          m_u8;
                    u16*         m_u16;
                    u32*         m_u32;
                    u64*         m_u64;
                    i8*          m_i8;
                    i16*         m_i16;
                    i32*         m_i32;
                    i64*         m_i64;
                    f32*         m_f32;
                    char*        m_char;
                    const char** m_str;
                };
                u16 m_type;
                u16 m_value_type;
                u32 m_maxlen;
            };

            void register_member(decoder_t* d, const char* name, carray_type_t carraytype);

            struct array_type_t
            {
                array_type_t(bool** out_value, i32 max_len = -1);
                array_type_t(u8** out_value, i32 max_len = -1);
                array_type_t(u16** out_value, i32 max_len = -1);
                array_type_t(u32** out_value, i32 max_len = -1);
                array_type_t(u64** out_value, i32 max_len = -1);
                array_type_t(i8** out_value, i32 max_len = -1);
                array_type_t(i16** out_value, i32 max_len = -1);
                array_type_t(i32** out_value, i32 max_len = -1);
                array_type_t(i64** out_value, i32 max_len = -1);
                array_type_t(f32** out_value, i32 max_len = -1);
                array_type_t(char** out_value, i32 max_len = -1);
                array_type_t(const char*** out_value, i32 max_len = -1);

                union
                {
                    void**        m_void;
                    bool**        m_bool;
                    u8**          m_u8;
                    u16**         m_u16;
                    u32**         m_u32;
                    u64**         m_u64;
                    i8**          m_i8;
                    i16**         m_i16;
                    i32**         m_i32;
                    i64**         m_i64;
                    f32**         m_f32;
                    char**        m_char;
                    const char*** m_str;
                };
                u32 m_type;
                u32 m_maxlen;
            };

            void register_member(decoder_t* d, const char* name, array_type_t arraytype, u8* out_array_size);
            void register_member(decoder_t* d, const char* name, array_type_t arraytype, u16* out_array_size);
            void register_member(decoder_t* d, const char* name, array_type_t arraytype, u32* out_array_size);
            void register_member(decoder_t* d, const char* name, array_type_t arraytype, u64* out_array_size);
            void register_member(decoder_t* d, const char* name, array_type_t arraytype, i8* out_array_size);
            void register_member(decoder_t* d, const char* name, array_type_t arraytype, i16* out_array_size);
            void register_member(decoder_t* d, const char* name, array_type_t arraytype, i32* out_array_size);
            void register_member(decoder_t* d, const char* name, array_type_t arraytype, i64* out_array_size);

            struct decoder_enum_t
            {
                const char** m_enum_strs;
                union
                {
                    void const*       m_values_void;
                    ncore::u8 const*  m_values_u8;
                    ncore::u16 const* m_values_u16;
                    ncore::u32 const* m_values_u32;
                    ncore::u64 const* m_values_u64;
                };
                i32 m_enum_count;
                i16 m_value_type;
                i16 m_as_flags;

                decoder_enum_t(const char** enum_strs, const u8* enum_values, i32 enum_count, bool as_flags = false)
                    : m_enum_strs(enum_strs)
                    , m_values_u8(enum_values)
                    , m_enum_count(enum_count)
                    , m_value_type(TYPE_U8)
                    , m_as_flags(as_flags ? 1 : 0)
                {
                }
                decoder_enum_t(const char** enum_strs, const u16* enum_values, i32 enum_count, bool as_flags = false)
                    : m_enum_strs(enum_strs)
                    , m_values_u16(enum_values)
                    , m_enum_count(enum_count)
                    , m_value_type(TYPE_U16)
                    , m_as_flags(as_flags ? 1 : 0)
                {
                }
                decoder_enum_t(const char** enum_strs, const u32* enum_values, i32 enum_count, bool as_flags = false)
                    : m_enum_strs(enum_strs)
                    , m_values_u32(enum_values)
                    , m_enum_count(enum_count)
                    , m_value_type(TYPE_U32)
                    , m_as_flags(as_flags ? 1 : 0)
                {
                }
                decoder_enum_t(const char** enum_strs, const u64* enum_values, i32 enum_count, bool as_flags = false)
                    : m_enum_strs(enum_strs)
                    , m_values_u64(enum_values)
                    , m_enum_count(enum_count)
                    , m_value_type(TYPE_U64)
                    , m_as_flags(as_flags ? 1 : 0)
                {
                }
            };

            void register_member(decoder_t* d, const char* name, decoder_enum_t const* decoder_enum, u8* out_value);
            void register_member(decoder_t* d, const char* name, decoder_enum_t const* decoder_enum, u16* out_value);
            void register_member(decoder_t* d, const char* name, decoder_enum_t const* decoder_enum, u32* out_value);
            void register_member(decoder_t* d, const char* name, decoder_enum_t const* decoder_enum, u64* out_value);

            result_t read_object_begin(decoder_t* d);
            result_t read_object_end(decoder_t* d);
            result_t read_array_begin(decoder_t* d, i32& out_size);
            result_t read_array_end(decoder_t* d);

            struct field_t
            {
                field_t(const char* name, const char* end)
                    : m_Name(name)
                    , m_End(end)
                {
                }
                const char* m_Name;
                const char* m_End;

                inline bool is_valid() const { return m_Name != nullptr && m_End != nullptr; }
            };
            field_t decode_field(decoder_t* d);
            bool    field_equal(const field_t& field, const char* cmp_name);

            // @return: true if member was found and decoded, false otherwise
            bool decoder_decode_member(decoder_t* d, field_t const& field);

            // Accept a lambda for decoding object
            template <typename TDecodeFn> void decode_object(decoder_t* d, const TDecodeFn* decode_fn)
            {
                result_t result = read_object_begin(d);
                while (OkAndNotEnded(result))
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
                    out_value = decode_i32(d);
                    return true;
                }
                return false;
            }

            inline bool field_set_f32(decoder_t* d, const field_t& field, const char* name, f32& out_value)
            {
                if (field_equal(field, name))
                {
                    out_value = decode_f32(d);
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

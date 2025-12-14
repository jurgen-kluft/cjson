#include "cbase/c_allocator.h"
#include "cbase/c_context.h"
#include "cbase/c_memory.h"
#include "cbase/c_printf.h"
#include "cbase/c_runes.h"

#include "cjson/c_json_utils.h"
#include "cjson/c_json_allocator.h"
#include "cjson/c_json_decoder.h"
#include "cjson/c_json_scanner.h"

namespace ncore
{
    namespace njson
    {
        namespace ndecoder
        {
            struct member_t;

            struct state_t
            {
                state_t*                              m_Parent;
                nscanner::JsonValue const*            m_Value;
                i32                                   m_Index; // Current index in iteration
                i32                                   m_Size;  // Total size of members/elements
                nscanner::JsonLinkedNamedValue const* m_ObjectMember;
                nscanner::JsonLinkedValue const*      m_ArrayElement;
                i32                                   m_MemberCount;
                member_t*                             m_Members;

                // The state of the stack allocator before this state was created, this is
                // to restore the stack allocator to this size when this state is done.
                s64   m_StackAllocatorSize;
                char* m_StackAllocatorEnd;

                void reset(state_t* parent, s64 stackSize, nscanner::JsonValue const* value)
                {
                    m_Parent             = parent;
                    m_Value              = value;
                    m_Index              = -1;
                    m_Size               = -1;
                    m_ObjectMember       = nullptr;
                    m_ArrayElement       = nullptr;
                    m_MemberCount        = 0;
                    m_Members            = nullptr;
                    m_StackAllocatorSize = stackSize;
                    m_StackAllocatorEnd  = nullptr;
                }
            };

            state_t* push_state(decoder_t* d, state_t* parent, nscanner::JsonValue const* value)
            {
                const s64 stackSize = d->m_StackAllocator->m_Size;
                state_t*  new_state = d->m_StackAllocator->Allocate<state_t>();
                new_state->reset(parent, stackSize, value);
                return new_state;
            }

            bool pop_state(decoder_t* d)
            {
                state_t* old_state          = d->m_CurrentState;
                d->m_StackAllocator->m_Size = old_state->m_StackAllocatorSize;
                d->m_CurrentState           = old_state->m_Parent;
                return (d->m_CurrentState != nullptr);
            }

            decoder_t* create_decoder(JsonAllocator* stack_allocator, JsonAllocator* decoder_allocator, const char* json, const char* json_end)
            {
                const char*                errmsg = nullptr;
                nscanner::JsonValue const* root   = nscanner::Scan(json, json_end, stack_allocator, errmsg);
                if (errmsg != nullptr)
                    return nullptr;

                decoder_t* d          = stack_allocator->Allocate<decoder_t>();
                d->m_StackAllocator   = stack_allocator;
                d->m_DecoderAllocator = decoder_allocator;

                const s64 stackSize = d->m_StackAllocator->m_Size;
                d->m_CurrentState   = stack_allocator->Allocate<state_t>();
                d->m_CurrentState->reset(nullptr, stackSize, root);

                return d;
            }

            void destroy_decoder(decoder_t*& d) {}

            void decode_bool(decoder_t* d, bool& out_value)
            {
                const nscanner::JsonNumberValue* number = nullptr;
                state_t*                         state  = d->m_CurrentState;
                if (state->m_Value != nullptr && state->m_Value->IsBoolean())
                    number = state->m_Value->AsNumber();
                out_value = (number != nullptr) ? ParseBoolean(number->m_String, number->m_End) : false;
            }

            void decode_i64(decoder_t* d, i64& out_value)
            {
                const nscanner::JsonNumberValue* number = nullptr;
                state_t*                         state  = d->m_CurrentState;
                if (state->m_Value != nullptr && state->m_Value->IsNumber())
                {
                    number = state->m_Value->AsNumber();
                }
                JsonNumber json_number;
                if (number != nullptr)
                {
                    const char* str = number->m_String;
                    ParseNumber(str, number->m_End, json_number);
                }
                out_value = JsonNumberAsInt64(json_number);
            }
            void decode_i8(decoder_t* d, i8& out_value)
            {
                i64 temp_value;
                decode_i64(d, temp_value);
                out_value = (i8)temp_value;
            }

            void decode_i16(decoder_t* d, i16& out_value)
            {
                i64 temp_value;
                decode_i64(d, temp_value);
                out_value = (i16)temp_value;
            }

            void decode_i32(decoder_t* d, i32& out_value)
            {
                i64 temp_value;
                decode_i64(d, temp_value);
                out_value = (i32)temp_value;
            }

            void decode_u64(decoder_t* d, u64& out_value)
            {
                const nscanner::JsonNumberValue* number = nullptr;
                state_t*                         state  = d->m_CurrentState;
                if (state->m_Value != nullptr && state->m_Value->IsNumber())
                {
                    number = state->m_Value->AsNumber();
                }
                JsonNumber json_number;
                if (number != nullptr)
                {
                    const char* str = number->m_String;
                    ParseNumber(str, number->m_End, json_number);
                }
                out_value = JsonNumberAsUInt64(json_number);
            }
            void decode_u8(decoder_t* d, u8& out_value)
            {
                u64 temp_value;
                decode_u64(d, temp_value);
                out_value = (u8)temp_value;
            }

            void decode_u16(decoder_t* d, u16& out_value)
            {
                u64 temp_value;
                decode_u64(d, temp_value);
                out_value = (u16)temp_value;
            }

            void decode_u32(decoder_t* d, u32& out_value)
            {
                u64 temp_value;
                decode_u64(d, temp_value);
                out_value = (u32)temp_value;
            }

            void decode_f32(decoder_t* d, f32& out_value)
            {
                const nscanner::JsonNumberValue* number = nullptr;
                state_t*                         state  = d->m_CurrentState;
                if (state->m_Value != nullptr && state->m_Value->IsNumber())
                {
                    number = state->m_Value->AsNumber();
                }
                JsonNumber json_number;
                if (number != nullptr)
                {
                    const char* str = number->m_String;
                    ParseNumber(str, number->m_End, json_number);
                }
                out_value = (f32)JsonNumberAsFloat64(json_number);
            }

            void decode_string(decoder_t* d, const char*& out_str)
            {
                const nscanner::JsonStringValue* str_value = nullptr;
                state_t*                         state     = d->m_CurrentState;
                if (state->m_Value != nullptr && state->m_Value->IsString())
                    str_value = state->m_Value->AsString();
                if (str_value != nullptr)
                {
                    const char* str_start = str_value->m_String;
                    const char* str_end   = str_value->m_End;
                    const u32   str_len   = (u32)(str_end - str_start);
                    char*       str       = d->m_DecoderAllocator->AllocateArray<char>(str_len + 1);
                    nmem::memcpy(str, str_start, str_len);
                    str[str_len] = 0;
                    out_str      = str;
                }
                else
                {
                    out_str = "";
                }
            }

            void decode_cstring(decoder_t* d, char* out_str, i32 out_str_len)
            {
                if (out_str_len == 0)
                    return;
                const nscanner::JsonStringValue* str_value = nullptr;
                state_t*                         state     = d->m_CurrentState;
                if (state->m_Value != nullptr && state->m_Value->IsString())
                    str_value = state->m_Value->AsString();
                if (str_value != nullptr)
                {
                    const char* str_start = str_value->m_String;
                    const char* str_end   = str_value->m_End;
                    const i32   str_len   = str_end - str_start;
                    const i32   copy_len  = (str_len < (out_str_len - 1)) ? str_len : (out_str_len - 1);
                    nmem::memcpy(out_str, str_start, copy_len);
                    out_str[copy_len] = 0;
                }
                else
                {
                    out_str[0] = 0;
                }
            }

            template <typename T> void decode_array_integer(decoder_t* d, T*& out_array, i32& out_array_size, i32 out_array_maxsize)
            {
                result_t result = read_array_begin(d, out_array_size);
                if (!result.valid())
                {
                    out_array      = nullptr;
                    out_array_size = 0;
                    return;
                }
                if (out_array_maxsize > 0)
                {
                    out_array_size = (out_array_size > (i32)out_array_maxsize) ? (i32)out_array_maxsize : out_array_size;
                }
                out_array = d->m_DecoderAllocator->AllocateArray<T>(out_array_size);

                i32 array_index = 0;
                while (result.valid())
                {
                    u64 value;
                    decode_u64(d, value);
                    if (array_index < (i32)out_array_size)
                        out_array[array_index] = (T)value;
                    array_index++;
                    result = read_array_end(d);
                }
            }

            void decode_array_bool(decoder_t* d, bool*& out_array, i32& out_array_size, i32 out_array_maxsize) { decode_array_integer<bool>(d, out_array, out_array_size, out_array_maxsize); }
            void decode_array_u8(decoder_t* d, u8*& out_array, i32& out_array_size, i32 out_array_maxsize) { decode_array_integer<u8>(d, out_array, out_array_size, out_array_maxsize); }
            void decode_array_u16(decoder_t* d, u16*& out_array, i32& out_array_size, i32 out_array_maxsize) { decode_array_integer<u16>(d, out_array, out_array_size, out_array_maxsize); }
            void decode_array_u32(decoder_t* d, u32*& out_array, i32& out_array_size, i32 out_array_maxsize) { decode_array_integer<u32>(d, out_array, out_array_size, out_array_maxsize); }
            void decode_array_u64(decoder_t* d, u64*& out_array, i32& out_array_size, i32 out_array_maxsize) { decode_array_integer<u64>(d, out_array, out_array_size, out_array_maxsize); }
            void decode_array_i8(decoder_t* d, i8*& out_array, i32& out_array_size, i32 out_array_maxsize) { decode_array_integer<i8>(d, out_array, out_array_size, out_array_maxsize); }
            void decode_array_i16(decoder_t* d, i16*& out_array, i32& out_array_size, i32 out_array_maxsize) { decode_array_integer<i16>(d, out_array, out_array_size, out_array_maxsize); }
            void decode_array_i32(decoder_t* d, i32*& out_array, i32& out_array_size, i32 out_array_maxsize) { decode_array_integer<i32>(d, out_array, out_array_size, out_array_maxsize); }
            void decode_array_i64(decoder_t* d, i64*& out_array, i32& out_array_size, i32 out_array_maxsize) { decode_array_integer<i64>(d, out_array, out_array_size, out_array_maxsize); }
            void decode_array_char(decoder_t* d, char*& out_array, i32& out_array_size, i32 out_array_maxsize) { decode_array_integer<char>(d, out_array, out_array_size, out_array_maxsize); }

            void decode_array_f32(decoder_t* d, f32*& out_array, i32& out_array_size, i32 out_array_maxsize)
            {
                i32      array_size;
                result_t result = read_array_begin(d, array_size);
                array_size      = (array_size > (i32)out_array_maxsize) ? (i32)out_array_maxsize : array_size;
                out_array_size  = (u32)array_size;
                out_array       = d->m_DecoderAllocator->AllocateArray<f32>(out_array_size);
                i32 array_index = 0;
                while (result.ok() && result.not_end())
                {
                    float color_component;
                    decode_f32(d, color_component);
                    if (array_index < (i32)out_array_size)
                        out_array[array_index] = color_component;
                    array_index++;
                    result = read_array_end(d);
                }
            }

            template <typename T> void decode_carray_integer(decoder_t* d, T* out_array, i32 out_array_maxlen)
            {
                i32      array_size;
                result_t result = read_array_begin(d, array_size);
                if (!result.valid())
                    return;
                i32 array_index = 0;
                while (result.ok() && result.not_end())
                {
                    u64 value;
                    decode_u64(d, value);
                    if (array_index < (i32)out_array_maxlen)
                        out_array[array_index] = (T)value;
                    array_index++;
                    result = read_array_end(d);
                }
            }

            void decode_carray_bool(decoder_t* d, bool* out_array, i32 out_array_maxlen) { decode_carray_integer<bool>(d, out_array, out_array_maxlen); }
            void decode_carray_u8(decoder_t* d, u8* out_array, i32 out_array_maxlen) { decode_carray_integer<u8>(d, out_array, out_array_maxlen); }
            void decode_carray_u16(decoder_t* d, u16* out_array, i32 out_array_maxlen) { decode_carray_integer<u16>(d, out_array, out_array_maxlen); }
            void decode_carray_u32(decoder_t* d, u32* out_array, i32 out_array_maxlen) { decode_carray_integer<u32>(d, out_array, out_array_maxlen); }
            void decode_carray_u64(decoder_t* d, u64* out_array, i32 out_array_maxlen) { decode_carray_integer<u64>(d, out_array, out_array_maxlen); }
            void decode_carray_i8(decoder_t* d, i8* out_array, i32 out_array_maxlen) { decode_carray_integer<i8>(d, out_array, out_array_maxlen); }
            void decode_carray_i16(decoder_t* d, i16* out_array, i32 out_array_maxlen) { decode_carray_integer<i16>(d, out_array, out_array_maxlen); }
            void decode_carray_i32(decoder_t* d, i32* out_array, i32 out_array_maxlen) { decode_carray_integer<i32>(d, out_array, out_array_maxlen); }
            void decode_carray_i64(decoder_t* d, i64* out_array, i32 out_array_maxlen) { decode_carray_integer<i64>(d, out_array, out_array_maxlen); }
            void decode_carray_char(decoder_t* d, char* out_array, i32 out_array_maxlen) { decode_carray_integer<char>(d, out_array, out_array_maxlen); }

            void decode_carray_f32(decoder_t* d, f32* out_array, i32 out_array_maxlen)
            {
                i32      array_size;
                result_t result      = read_array_begin(d, array_size);
                i32      array_index = 0;
                while (result.ok() && result.not_end())
                {
                    float color_component;
                    decode_f32(d, color_component);
                    if (array_index < (i32)out_array_maxlen)
                        out_array[array_index] = color_component;
                    array_index++;
                    result = read_array_end(d);
                }
            }

            void decode_enum(decoder_t* d, u8& out_enum_value, const char** enum_strs, const u8* enum_values, i32 enum_count, bool as_flags)
            {
                // TODO
                // Not implemented yet
            }
            void decode_enum(decoder_t* d, u16& out_enum_value, const char** enum_strs, const u16* enum_values, i32 enum_count, bool as_flags)
            {
                // TODO
                // Not implemented yet
            }
            void decode_enum(decoder_t* d, u32& out_enum_value, const char** enum_strs, const u32* enum_values, i32 enum_count, bool as_flags)
            {
                // TODO
                // Not implemented yet
            }
            void decode_enum(decoder_t* d, u64& out_enum_value, const char** enum_strs, const u64* enum_values, i32 enum_count, bool as_flags)
            {
                // TODO
                // Not implemented yet
            }

            result_t read_object_begin(decoder_t* d)
            {
                if (d->m_CurrentState == nullptr)
                    return result_t(false, true);
                if (!d->m_CurrentState->m_Value->IsObject())
                    return result_t(false, true);
                if (d->m_CurrentState->m_Value->AsObject()->m_LinkedList == nullptr)
                    return result_t(true, true);

                state_t* new_state = push_state(d, d->m_CurrentState, nullptr);

                // Set first object member
                new_state->m_Index        = 0;
                new_state->m_Size         = d->m_CurrentState->m_Value->AsObject()->m_Count;
                new_state->m_ObjectMember = d->m_CurrentState->m_Value->AsObject()->m_LinkedList;

                d->m_CurrentState = new_state;

                if (new_state->m_ObjectMember != nullptr)
                    new_state->m_Value = new_state->m_ObjectMember->m_NamedValue->m_Value;

                return result_t(true, new_state->m_ObjectMember == nullptr);
            }

            result_t read_object_end(decoder_t* d)
            {
                ASSERT(d->m_CurrentState->m_ObjectMember != nullptr);

                d->m_CurrentState->m_ObjectMember = d->m_CurrentState->m_ObjectMember->m_Next;
                d->m_CurrentState->m_Index += 1;

                result_t result(true, d->m_CurrentState->m_ObjectMember == nullptr);
                if (result.end())
                {
                    d->m_CurrentState->m_Value = nullptr;
                    pop_state(d);
                    return result;
                }
                d->m_CurrentState->m_Value = d->m_CurrentState->m_ObjectMember->m_NamedValue->m_Value;
                return result;
            }

            result_t read_array_begin(decoder_t* d, i32& out_size)
            {
                if (d->m_CurrentState == nullptr)
                    return result_t(false, true);
                if (!d->m_CurrentState->m_Value->IsArray())
                    return result_t(false, true);
                if (d->m_CurrentState->m_Value->AsArray()->m_LinkedList == nullptr)
                    return result_t(true, true);

                state_t* new_state = push_state(d, d->m_CurrentState, nullptr);

                // Set first array element
                new_state->m_Index        = 0;
                new_state->m_Size         = d->m_CurrentState->m_Value->AsArray()->m_Count;
                new_state->m_ArrayElement = d->m_CurrentState->m_Value->AsArray()->m_LinkedList;

                d->m_CurrentState = new_state;

                if (new_state->m_ArrayElement != nullptr)
                {
                    new_state->m_Value = new_state->m_ArrayElement->m_Value;
                    out_size           = new_state->m_Size;
                    return result_t(true, false);
                }
                out_size = 0;
                return result_t(true, true);
            }

            result_t read_array_end(decoder_t* d)
            {
                ASSERT(d->m_CurrentState->m_ArrayElement != nullptr);

                d->m_CurrentState->m_ArrayElement = d->m_CurrentState->m_ArrayElement->m_Next;
                d->m_CurrentState->m_Index += 1;

                // Return whether we are at the end of this array
                result_t result(true, d->m_CurrentState->m_ArrayElement == nullptr);
                if (result.end())
                {
                    d->m_CurrentState->m_Value = nullptr;
                    pop_state(d);
                    return result;
                }

                d->m_CurrentState->m_Value = d->m_CurrentState->m_ArrayElement->m_Value;
                return result;
            }

            // ----------------------------------------------------------------------------------------------------------------------------------------
            // ----------------------------------------------------------------------------------------------------------------------------------------
            // ----------------------------------------------------------------------------------------------------------------------------------------
            // Member type definitions
            enum EMemberType
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
                TYPE_CHAR_ARRAY
            };

            enum EMemberStructureType
            {
                STRUCTURE_TYPE_INVALID = 0,
                STRUCTURE_TYPE_BASIC,
                STRUCTURE_TYPE_ARRAY,
                STRUCTURE_TYPE_ENUM
            };

            // ----------------------------------------------------------------------------------------------------------------------------------------
            // ----------------------------------------------------------------------------------------------------------------------------------------
            // ----------------------------------------------------------------------------------------------------------------------------------------
            // Member structures
            struct member_basic_t
            {
                const char* m_name;    // member name
                i32         m_count;   // for array's, the maximum number of elements in the array
                i8          m_info[4]; // member type[2] / structure type[3]

                union
                {
                    // single values or fixed size array's
                    void*        m_member_void;
                    bool*        m_member_bool;
                    ncore::s8*   m_member_i8;
                    ncore::s16*  m_member_i16;
                    ncore::i32*  m_member_i32;
                    ncore::i64*  m_member_i64;
                    ncore::u8*   m_member_u8;
                    ncore::u16*  m_member_u16;
                    ncore::u32*  m_member_u32;
                    ncore::u64*  m_member_u64;
                    float*       m_member_f32;
                    char*        m_member_char;
                    const char** m_member_string;
                };
            };
            struct member_array_t
            {
                const char* m_name;    // member name
                i32         m_size;    // -1 == unbounded, otherwise maximum size of the array
                i8          m_info[4]; // size type[1] / member type[2] / structure type[3]

                union
                {
                    void**       m_void_array;
                    bool**       m_bool_array;
                    ncore::s8**  m_i8_array;
                    ncore::s16** m_i16_array;
                    ncore::i32** m_i32_array;
                    ncore::i64** m_i64_array;
                    ncore::u8**  m_u8_array;
                    ncore::u16** m_u16_array;
                    ncore::u32** m_u32_array;
                    ncore::u64** m_u64_array;
                    float**      m_f32_array;
                    char**       m_char_array;
                };

                union
                {
                    // for dynamic array's, pointer to member that holds the size of the array
                    void*       m_void_size;
                    ncore::s8*  m_i8_size;
                    ncore::s16* m_i16_size;
                    ncore::i32* m_i32_size;
                    ncore::i64* m_i64_size;
                    ncore::u8*  m_u8_size;
                    ncore::u16* m_u16_size;
                    ncore::u32* m_u32_size;
                    ncore::u64* m_u64_size;
                };
            };
            struct member_enum_t
            {
                const char* m_name;       // member name
                i32         m_enum_count; // number of enum values
                i8          m_info[4];    // is flags[0] / member type[2] / structure type[3]

                const char** m_enum_strs; // array of enum strings
                union
                {
                    void*       m_values_void;
                    ncore::u8*  m_values_u8;  // array of enum values, u8
                    ncore::u16* m_values_u16; // array of enum values, u16
                    ncore::u32* m_values_u32; // array of enum values, u32
                    ncore::u64* m_values_u64; // array of enum values, u64
                };
            };

            struct member_t
            {
                union
                {
                    member_basic_t m_basic;
                    member_array_t m_array;
                    member_enum_t  m_enum;
                };
            };

            // ----------------------------------------------------------------------------------------------------------------------------------------
            // ----------------------------------------------------------------------------------------------------------------------------------------
            // ----------------------------------------------------------------------------------------------------------------------------------------
            // Decode field, this will return the name begin and end pointer of the current member name
            field_t decode_field(decoder_t* d)
            {
                state_t* state = d->m_CurrentState;
                if (state->m_StackAllocatorEnd != nullptr)
                {
                    // Finalize the 'checkout' that was done for the array of members
                    char* commit_ptr = (char*)&state->m_Members[state->m_MemberCount];
                    d->m_StackAllocator->Commit(commit_ptr);
                    state->m_StackAllocatorEnd = nullptr;
                }

                field_t field;
                if (state->m_ObjectMember != nullptr)
                {
                    field.m_Name = state->m_ObjectMember->m_NamedValue->m_Name->m_String;
                    field.m_End  = state->m_ObjectMember->m_NamedValue->m_Name->m_End;
                }
                else
                {
                    field.m_Name = nullptr;
                    field.m_End  = nullptr;
                }
                return field;
            }

            // ----------------------------------------------------------------------------------------------------------------------------------------
            // ----------------------------------------------------------------------------------------------------------------------------------------
            // ----------------------------------------------------------------------------------------------------------------------------------------
            // Basic members, single values or fixed size array's

            static void set_basic_member(decoder_t* d, const char* name, i32 size_max, i16 member_type, void* member_ptr)
            {
                if (d->m_CurrentState->m_Members == nullptr)
                {
                    d->m_CurrentState->m_Members     = (member_t*)d->m_StackAllocator->CheckOut(d->m_CurrentState->m_StackAllocatorEnd);
                    d->m_CurrentState->m_MemberCount = 0;
                }
                member_t* m              = &d->m_CurrentState->m_Members[d->m_CurrentState->m_MemberCount++];
                m->m_basic.m_name        = name;
                m->m_basic.m_count       = size_max;
                m->m_basic.m_info[2]     = member_type;
                m->m_basic.m_info[3]     = STRUCTURE_TYPE_BASIC;
                m->m_basic.m_member_void = member_ptr;
            }

            void decoder_add_member(decoder_t* d, const char* name, bool* out_value, i32 out_value_maxlen) { set_basic_member(d, name, out_value_maxlen, TYPE_BOOL, out_value); }
            void decoder_add_member(decoder_t* d, const char* name, u8* out_value, i32 out_value_maxlen) { set_basic_member(d, name, out_value_maxlen, TYPE_U8, out_value); }
            void decoder_add_member(decoder_t* d, const char* name, u16* out_value, i32 out_value_maxlen) { set_basic_member(d, name, out_value_maxlen, TYPE_U16, out_value); }
            void decoder_add_member(decoder_t* d, const char* name, u32* out_value, i32 out_value_maxlen) { set_basic_member(d, name, out_value_maxlen, TYPE_U32, out_value); }
            void decoder_add_member(decoder_t* d, const char* name, u64* out_value, i32 out_value_maxlen) { set_basic_member(d, name, out_value_maxlen, TYPE_U64, out_value); }
            void decoder_add_member(decoder_t* d, const char* name, i8* out_value, i32 out_value_maxlen) { set_basic_member(d, name, out_value_maxlen, TYPE_I8, out_value); }
            void decoder_add_member(decoder_t* d, const char* name, i16* out_value, i32 out_value_maxlen) { set_basic_member(d, name, out_value_maxlen, TYPE_I16, out_value); }
            void decoder_add_member(decoder_t* d, const char* name, i32* out_value, i32 out_value_maxlen) { set_basic_member(d, name, out_value_maxlen, TYPE_I32, out_value); }
            void decoder_add_member(decoder_t* d, const char* name, i64* out_value, i32 out_value_maxlen) { set_basic_member(d, name, out_value_maxlen, TYPE_I64, out_value); }
            void decoder_add_member(decoder_t* d, const char* name, f32* out_value, i32 out_value_maxlen) { set_basic_member(d, name, out_value_maxlen, TYPE_F32, out_value); }
            void decoder_add_member(decoder_t* d, const char* name, char* out_value, i32 out_value_maxlen) { set_basic_member(d, name, out_value_maxlen, TYPE_CHAR, out_value); }
            void decoder_add_member(decoder_t* d, const char* name, const char** out_value, i32 out_value_maxlen) { set_basic_member(d, name, out_value_maxlen, TYPE_STRING, out_value); }

            // ----------------------------------------------------------------------------------------------------------------------------------------
            // ----------------------------------------------------------------------------------------------------------------------------------------
            // ----------------------------------------------------------------------------------------------------------------------------------------
            // Array members

            static void set_array_member(decoder_t* d, const char* name, i32 size_max, i16 member_type, void** array_ptr, i16 size_type, void* size_ptr)
            {
                if (d->m_CurrentState->m_Members == nullptr)
                {
                    d->m_CurrentState->m_Members     = (member_t*)d->m_StackAllocator->CheckOut(d->m_CurrentState->m_StackAllocatorEnd);
                    d->m_CurrentState->m_MemberCount = 0;
                }
                member_t* m             = &d->m_CurrentState->m_Members[d->m_CurrentState->m_MemberCount++];
                m->m_array.m_name       = name;
                m->m_array.m_info[1]    = size_type;
                m->m_array.m_info[2]    = member_type;
                m->m_array.m_info[3]    = STRUCTURE_TYPE_ARRAY;
                m->m_array.m_void_array = array_ptr;
                m->m_array.m_size       = size_max; // -1 == unbounded, otherwise maximum size of the array
            }

            void decoder_add_member(decoder_t* d, const char* name, u8** out_value, i32* out_len, i32 max_len) { set_array_member(d, name, max_len, TYPE_U8_ARRAY, (void**)out_value, TYPE_I32, out_len); }
            void decoder_add_member(decoder_t* d, const char* name, u16** out_value, i32* out_len, i32 max_len) { set_array_member(d, name, max_len, TYPE_U16_ARRAY, (void**)out_value, TYPE_I32, out_len); }
            void decoder_add_member(decoder_t* d, const char* name, u32** out_value, i32* out_len, i32 max_len) { set_array_member(d, name, max_len, TYPE_U32_ARRAY, (void**)out_value, TYPE_I32, out_len); }
            void decoder_add_member(decoder_t* d, const char* name, u64** out_value, i32* out_len, i32 max_len) { set_array_member(d, name, max_len, TYPE_U64_ARRAY, (void**)out_value, TYPE_I32, out_len); }
            void decoder_add_member(decoder_t* d, const char* name, i8** out_value, i32* out_len, i32 max_len) { set_array_member(d, name, max_len, TYPE_I8_ARRAY, (void**)out_value, TYPE_I32, out_len); }
            void decoder_add_member(decoder_t* d, const char* name, i16** out_value, i32* out_len, i32 max_len) { set_array_member(d, name, max_len, TYPE_I16_ARRAY, (void**)out_value, TYPE_I32, out_len); }
            void decoder_add_member(decoder_t* d, const char* name, i32** out_value, i32* out_len, i32 max_len) { set_array_member(d, name, max_len, TYPE_I32_ARRAY, (void**)out_value, TYPE_I32, out_len); }
            void decoder_add_member(decoder_t* d, const char* name, i64** out_value, i32* out_len, i32 max_len) { set_array_member(d, name, max_len, TYPE_I64_ARRAY, (void**)out_value, TYPE_I32, out_len); }
            void decoder_add_member(decoder_t* d, const char* name, f32** out_value, i32* out_len, i32 max_len) { set_array_member(d, name, max_len, TYPE_F32_ARRAY, (void**)out_value, TYPE_I32, out_len); }
            void decoder_add_member(decoder_t* d, const char* name, f32** out_value, i8* out_len, i32 max_len) { set_array_member(d, name, max_len, TYPE_F32_ARRAY, (void**)out_value, TYPE_I8, out_len); }
            void decoder_add_member(decoder_t* d, const char* name, char** out_value, i32* out_len, i32 max_len) { set_array_member(d, name, max_len, TYPE_CHAR_ARRAY, (void**)out_value, TYPE_I32, out_len); }

            // ----------------------------------------------------------------------------------------------------------------------------------------
            // ----------------------------------------------------------------------------------------------------------------------------------------
            // ----------------------------------------------------------------------------------------------------------------------------------------
            // Enum members

            void decoder_add_enum_member(decoder_t* d, const char* name, const char** enum_strs, const void* enum_values, i8 sizeof_enum_value, i32 enum_count, bool as_flags, void* out_value, i8 sizeof_out_value)
            {
                if (d->m_CurrentState->m_Members == nullptr)
                {
                    d->m_CurrentState->m_Members     = (member_t*)d->m_StackAllocator->CheckOut(d->m_CurrentState->m_StackAllocatorEnd);
                    d->m_CurrentState->m_MemberCount = 0;
                }
                member_t* m             = &d->m_CurrentState->m_Members[d->m_CurrentState->m_MemberCount++];
                m->m_enum.m_name        = name;
                m->m_enum.m_enum_count  = enum_count;
                m->m_enum.m_info[0]     = as_flags ? 1 : 0;
                m->m_enum.m_info[2]     = TYPE_U8 + sizeof_enum_value;
                m->m_enum.m_info[3]     = STRUCTURE_TYPE_ENUM;
                m->m_enum.m_enum_strs   = enum_strs;
                m->m_enum.m_values_void = (void*)enum_values;
            }

            // ----------------------------------------------------------------------------------------------------------------------------------------
            // ----------------------------------------------------------------------------------------------------------------------------------------
            // ----------------------------------------------------------------------------------------------------------------------------------------
            // Member based decoding
            // @return: true if member was found and decoded, false otherwise

            bool decoder_decode_member(decoder_t* d, field_t const& field)
            {
                state_t*  state  = d->m_CurrentState;
                member_t* member = nullptr;
                for (i32 i = 0; i < state->m_MemberCount; i++)
                {
                    if (field_equal(field, state->m_Members[i].m_basic.m_name))
                    {
                        member = &state->m_Members[i];
                        break;
                    }
                }
                if (member != nullptr)
                {
                    // Found the member, decode based on type
                    switch (member->m_basic.m_info[3]) // structure type
                    {
                        case STRUCTURE_TYPE_BASIC:
                        {
                            switch (member->m_basic.m_info[2]) // member type
                            {
                                case TYPE_BOOL:
                                {
                                    bool* out_value = member->m_basic.m_member_bool;
                                    decode_bool(d, *out_value);
                                    return true;
                                }
                                case TYPE_I8:
                                {
                                    i8* out_value = member->m_basic.m_member_i8;
                                    decode_i8(d, *out_value);
                                    return true;
                                }
                                case TYPE_I16:
                                {
                                    i16* out_value = member->m_basic.m_member_i16;
                                    decode_i16(d, *out_value);
                                    return true;
                                }
                                case TYPE_I32:
                                {
                                    i32* out_value = member->m_basic.m_member_i32;
                                    decode_i32(d, *out_value);
                                    return true;
                                }
                                case TYPE_I64:
                                {
                                    i64* out_value = member->m_basic.m_member_i64;
                                    decode_i64(d, *out_value);
                                    return true;
                                }
                                case TYPE_U8:
                                {
                                    u8* out_value = member->m_basic.m_member_u8;
                                    decode_u8(d, *out_value);
                                    return true;
                                }
                                case TYPE_U16:
                                {
                                    u16* out_value = member->m_basic.m_member_u16;
                                    decode_u16(d, *out_value);
                                    return true;
                                }
                                case TYPE_U32:
                                {
                                    u32* out_value = member->m_basic.m_member_u32;
                                    decode_u32(d, *out_value);
                                    return true;
                                }
                                case TYPE_U64:
                                {
                                    u64* out_value = member->m_basic.m_member_u64;
                                    decode_u64(d, *out_value);
                                    return true;
                                }
                                case TYPE_F32:
                                {
                                    f32* out_value = member->m_basic.m_member_f32;
                                    decode_f32(d, *out_value);
                                    return true;
                                }
                                case TYPE_STRING:
                                {
                                    const char** out_value = member->m_basic.m_member_string;
                                    decode_string(d, *out_value);
                                    return true;
                                }
                                default: return false;
                            }
                            break;
                        }
                        case STRUCTURE_TYPE_ARRAY:
                        {
                            i32 out_array_size = 0;
                            switch (member->m_array.m_info[2]) // member type
                            {
                                case TYPE_BOOL_ARRAY:
                                {
                                    bool** out_array = member->m_array.m_bool_array;
                                    decode_array_bool(d, *out_array, out_array_size, member->m_array.m_size);
                                }
                                break;
                                case TYPE_I8_ARRAY:
                                {
                                    i8** out_array = member->m_array.m_i8_array;
                                    decode_array_i8(d, *out_array, out_array_size, member->m_array.m_size);
                                }
                                break;
                                case TYPE_I16_ARRAY:
                                {
                                    i16** out_array = member->m_array.m_i16_array;
                                    decode_array_i16(d, *out_array, out_array_size, member->m_array.m_size);
                                }
                                break;
                                case TYPE_I32_ARRAY:
                                {
                                    i32** out_array = member->m_array.m_i32_array;
                                    decode_array_i32(d, *out_array, out_array_size, member->m_array.m_size);
                                }
                                break;
                                case TYPE_I64_ARRAY:
                                {
                                    i64** out_array = member->m_array.m_i64_array;
                                    decode_array_i64(d, *out_array, out_array_size, member->m_array.m_size);
                                }
                                break;
                                case TYPE_U8_ARRAY:
                                {
                                    u8** out_array = member->m_array.m_u8_array;
                                    decode_array_u8(d, *out_array, out_array_size, member->m_array.m_size);
                                }
                                break;
                                case TYPE_U16_ARRAY:
                                {
                                    u16** out_array = member->m_array.m_u16_array;
                                    decode_array_u16(d, *out_array, out_array_size, member->m_array.m_size);
                                }
                                break;
                                case TYPE_U32_ARRAY:
                                {
                                    u32** out_array = member->m_array.m_u32_array;
                                    decode_array_u32(d, *out_array, out_array_size, member->m_array.m_size);
                                }
                                break;
                                case TYPE_U64_ARRAY:
                                {
                                    u64** out_array = member->m_array.m_u64_array;
                                    decode_array_u64(d, *out_array, out_array_size, member->m_array.m_size);
                                }
                                break;
                                case TYPE_F32_ARRAY:
                                {
                                    f32** out_array = member->m_array.m_f32_array;
                                    decode_array_f32(d, *out_array, out_array_size, member->m_array.m_size);
                                }
                                break;
                                case TYPE_CHAR_ARRAY:
                                {
                                    char** out_array = member->m_array.m_char_array;
                                    decode_array_char(d, *out_array, out_array_size, member->m_array.m_size);
                                }
                                break;
                                default: return false;
                            }

                            switch (member->m_array.m_info[1]) // size type
                            {
                                case TYPE_I8: out_array_size = (i8)out_array_size; break;
                                case TYPE_I16: out_array_size = (i16)out_array_size; break;
                                case TYPE_I32: out_array_size = (i32)out_array_size; break;
                                case TYPE_I64: out_array_size = (i64)out_array_size; break;
                                case TYPE_U8: out_array_size = (u8)out_array_size; break;
                                case TYPE_U16: out_array_size = (u16)out_array_size; break;
                                case TYPE_U32: out_array_size = (u32)out_array_size; break;
                                case TYPE_U64: out_array_size = (u64)out_array_size; break;
                                default: return false;
                            }

                            break;
                        }
                        case STRUCTURE_TYPE_ENUM:
                        {
                            const bool as_flags = (member->m_enum.m_info[0] != 0);
                            switch (member->m_enum.m_info[2]) // member type
                            {
                                case TYPE_U8:
                                {
                                    u8 out_enum_value = 0;
                                    decode_enum(d, out_enum_value, member->m_enum.m_enum_strs, member->m_enum.m_values_u8, member->m_enum.m_enum_count, as_flags);
                                    return true;
                                }
                                case TYPE_U16:
                                {
                                    u16 out_enum_value = 0;
                                    decode_enum(d, out_enum_value, member->m_enum.m_enum_strs, member->m_enum.m_values_u16, member->m_enum.m_enum_count, as_flags);
                                    return true;
                                }
                                case TYPE_U32:
                                {
                                    u32 out_enum_value = 0;
                                    decode_enum(d, out_enum_value, member->m_enum.m_enum_strs, member->m_enum.m_values_u32, member->m_enum.m_enum_count, as_flags);
                                    return true;
                                }
                                case TYPE_U64:
                                {
                                    u64 out_enum_value = 0;
                                    decode_enum(d, out_enum_value, member->m_enum.m_enum_strs, member->m_enum.m_values_u64, member->m_enum.m_enum_count, as_flags);
                                    return true;
                                }
                            }
                            break;
                        }
                    }
                }
                return false;
            }

        } // namespace ndecoder
    } // namespace njson
} // namespace ncore

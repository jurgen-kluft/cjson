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
                const s64 allocator_initial_size = stack_allocator->m_Size;

                const char*                errmsg = nullptr;
                nscanner::JsonValue const* root   = nscanner::Scan(json, json_end, stack_allocator, errmsg);
                if (errmsg != nullptr)
                    return nullptr;

                decoder_t* d                   = stack_allocator->Allocate<decoder_t>();
                d->m_StackAllocator            = stack_allocator;
                d->m_DecoderAllocator          = decoder_allocator;
                d->m_StackAllocatorInitialSize = allocator_initial_size;

                const s64 stackSize = d->m_StackAllocator->m_Size;
                d->m_CurrentState   = stack_allocator->Allocate<state_t>();
                d->m_CurrentState->reset(nullptr, stackSize, root);

                return d;
            }

            void destroy_decoder(decoder_t*& d)
            {
                if (d != nullptr)
                {
                    d->m_StackAllocator->m_Size = d->m_StackAllocatorInitialSize;
                    d                           = nullptr;
                }
            }

            bool decode_bool(decoder_t* d)
            {
                const nscanner::JsonNumberValue* number = nullptr;
                state_t*                         state  = d->m_CurrentState;
                if (state->m_Value != nullptr && state->m_Value->IsBoolean())
                    number = state->m_Value->AsNumber();
                return (number != nullptr) ? ParseBoolean(number->m_String, number->m_End) : false;
            }

            i64 decode_i64(decoder_t* d)
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
                return JsonNumberAsInt64(json_number);
            }
            i8  decode_i8(decoder_t* d) { return (i8)decode_i64(d); }
            i16 decode_i16(decoder_t* d) { return (i16)decode_i64(d); }
            i32 decode_i32(decoder_t* d) { return (i32)decode_i64(d); }

            u64 decode_u64(decoder_t* d)
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
                return JsonNumberAsUInt64(json_number);
            }
            u8  decode_u8(decoder_t* d) { return (u8)decode_u64(d); }
            u16 decode_u16(decoder_t* d) { return (u16)decode_u64(d); }
            u32 decode_u32(decoder_t* d) { return (u32)decode_u64(d); }

            f32 decode_f32(decoder_t* d)
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
               return (f32)JsonNumberAsFloat64(json_number);
            }

            char decode_char(decoder_t* d)
            {
                const nscanner::JsonStringValue* str_value = nullptr;
                state_t*                         state     = d->m_CurrentState;
                if (state->m_Value != nullptr && state->m_Value->IsString())
                    str_value = state->m_Value->AsString();
                if (str_value != nullptr)
                {
                    const char* str = str_value->m_String;
                    return *str;
                }
                return '\0';
            }

            char* decode_chars(decoder_t* d)
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
                    return str;
                }
                return nullptr;
            }

            const char* decode_string(decoder_t* d)
            {
                char* str = decode_chars(d);
                return (str != nullptr) ? str : "";
            }

            u64 decode_mac_addr(decoder_t* d)
            {
                const nscanner::JsonStringValue* str_value = nullptr;
                state_t*                         state     = d->m_CurrentState;
                if (state->m_Value != nullptr && state->m_Value->IsString())
                    str_value = state->m_Value->AsString();
                if (str_value != nullptr)
                {
                    const char* str_start = str_value->m_String;
                    const char* str_end   = str_value->m_End;
                    return ParseMacAddress(str_start, str_end);
                }
                return 0;
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
                if (NotOk(result))
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
                while (OkAndNotEnded(result))
                {
                    u64 value = decode_u64(d);
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
            void decode_array_f32(decoder_t* d, f32*& out_array, i32& out_array_size, i32 out_array_maxsize)
            {
                i32      array_size;
                result_t result = read_array_begin(d, array_size);
                array_size      = (array_size > (i32)out_array_maxsize) ? (i32)out_array_maxsize : array_size;
                out_array_size  = (u32)array_size;
                out_array       = d->m_DecoderAllocator->AllocateArray<f32>(out_array_size);
                i32 array_index = 0;
                while (OkAndNotEnded(result))
                {
                    float color_component =                     decode_f32(d);
                    if (array_index < (i32)out_array_size)
                        out_array[array_index] = color_component;
                    array_index++;
                    result = read_array_end(d);
                }
            }

            void decode_array_char(decoder_t* d, char*& out_array, i32& out_array_size, i32 out_array_maxsize) { decode_array_integer<char>(d, out_array, out_array_size, out_array_maxsize); }

            void decode_array_str(decoder_t* d, const char**& out_array, i32& out_array_size, i32 out_array_maxsize)
            {
                result_t result = read_array_begin(d, out_array_size);
                if (NotOk(result))
                {
                    out_array      = nullptr;
                    out_array_size = 0;
                    return;
                }
                if (out_array_maxsize > 0)
                {
                    out_array_size = (out_array_size > (i32)out_array_maxsize) ? (i32)out_array_maxsize : out_array_size;
                }
                out_array = d->m_DecoderAllocator->AllocateArray<const char*>(out_array_size);

                i32 array_index = 0;
                while (OkAndNotEnded(result))
                {
                    if (array_index < (i32)out_array_size)
                        out_array[array_index] = decode_string(d);
                    array_index++;
                    result = read_array_end(d);
                }
            }

            template <typename T> void decode_carray_integer(decoder_t* d, T* out_array, i32 out_array_maxlen)
            {
                i32      array_size;
                result_t result = read_array_begin(d, array_size);
                if (NotOk(result))
                    return;
                i32 array_index = 0;
                while (OkAndNotEnded(result))
                {
                    u64 value = decode_u64(d);
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
            void decode_carray_str(decoder_t* d, const char** out_array, i32 out_array_maxlen)
            {
                i32      array_size;
                result_t result = read_array_begin(d, array_size);
                if (NotOk(result))
                    return;
                i32 array_index = 0;
                while (OkAndNotEnded(result))
                {
                    if (array_index < (i32)out_array_maxlen)
                        out_array[array_index] = decode_string(d);
                    array_index++;
                    result = read_array_end(d);
                }
            }

            void decode_carray_f32(decoder_t* d, f32* out_array, i32 out_array_maxlen)
            {
                i32      array_size;
                result_t result      = read_array_begin(d, array_size);
                i32      array_index = 0;
                while (OkAndNotEnded(result))
                {
                    float color_component = decode_f32(d);
                    if (array_index < (i32)out_array_maxlen)
                        out_array[array_index] = color_component;
                    array_index++;
                    result = read_array_end(d);
                }
            }

            i32 decode_find_enum(decoder_t* d, const char** enum_strs, i32 enum_count)
            {
                state_t* state = d->m_CurrentState;
                if (state->m_Value != nullptr && state->m_Value->IsString())
                {
                    const nscanner::JsonStringValue* str_value = state->m_Value->AsString();
                    if (str_value != nullptr)
                    {
                        field_t field(str_value->m_String, str_value->m_End);
                        for (i32 i = 0; i < enum_count; i++)
                        {
                            if (field_equal(field, enum_strs[i]))
                            {
                                return i;
                            }
                        }
                    }
                }
                return -1;
            }

            // decode_find_flag: find a flag in a string separated by "|", this should be called repeatedly until all
            // flags are found flags appear in the string in the following format:
            //           "Flag1|Flag2|Flag3"
            // each flag is separated by a '|'
            bool decode_find_flag(const char** enum_strs, i32 enum_count, field_t& flag, i32& out_enum_index)
            {
                if (flag.m_Name >= flag.m_End)
                    return false;

                const char* flag_end = flag.m_Name;
                while (flag_end < flag.m_End && *flag_end != '|')
                    ++flag_end;

                field_t flag_field(flag.m_Name, flag_end);
                flag.m_Name = flag_end + 1; // Move to next flag

                // Compare with each enum string
                out_enum_index = -1;
                for (i32 i = 0; i < enum_count; i++)
                {
                    if (field_equal(flag_field, enum_strs[i]))
                    {
                        out_enum_index = i;
                        break;
                    }
                }
                return true;
            }

            field_t decode_string_as_field(decoder_t* d)
            {
                state_t* state = d->m_CurrentState;
                if (state->m_Value != nullptr && state->m_Value->IsString())
                {
                    const nscanner::JsonStringValue* str_value = state->m_Value->AsString();
                    if (str_value != nullptr)
                    {
                        return field_t(str_value->m_String, str_value->m_End);
                    }
                }
                return field_t(nullptr, nullptr);
            }

            result_t read_object_begin(decoder_t* d)
            {
                if (d->m_CurrentState == nullptr)
                    return ResultInvalid;
                if (!d->m_CurrentState->m_Value->IsObject())
                    return ResultInvalid;
                if (d->m_CurrentState->m_Value->AsObject()->m_LinkedList == nullptr)
                    return ResultOkAndEnded;

                state_t* new_state = push_state(d, d->m_CurrentState, nullptr);

                // Set first object member
                new_state->m_Index        = 0;
                new_state->m_Size         = d->m_CurrentState->m_Value->AsObject()->m_Count;
                new_state->m_ObjectMember = d->m_CurrentState->m_Value->AsObject()->m_LinkedList;

                d->m_CurrentState = new_state;

                if (new_state->m_ObjectMember != nullptr)
                    new_state->m_Value = new_state->m_ObjectMember->m_NamedValue->m_Value;

                return new_state->m_ObjectMember == nullptr ? ResultOkAndEnded : ResultOkAndNotEnded;
            }

            result_t read_object_end(decoder_t* d)
            {
                ASSERT(d->m_CurrentState->m_ObjectMember != nullptr);

                d->m_CurrentState->m_ObjectMember = d->m_CurrentState->m_ObjectMember->m_Next;
                d->m_CurrentState->m_Index += 1;

                result_t result = d->m_CurrentState->m_ObjectMember == nullptr ? ResultOkAndEnded : ResultOkAndNotEnded;
                if (Ended(result))
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
                    return ResultInvalid;
                if (!d->m_CurrentState->m_Value->IsArray())
                    return ResultInvalid;
                if (d->m_CurrentState->m_Value->AsArray()->m_LinkedList == nullptr)
                    return ResultOkAndEnded;

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
                    return ResultOkAndNotEnded;
                }
                out_size = 0;
                return ResultOkAndEnded;
            }

            result_t read_array_end(decoder_t* d)
            {
                ASSERT(d->m_CurrentState->m_ArrayElement != nullptr);

                d->m_CurrentState->m_ArrayElement = d->m_CurrentState->m_ArrayElement->m_Next;
                d->m_CurrentState->m_Index += 1;

                // Return whether we are at the end of this array
                const result_t result = (d->m_CurrentState->m_ArrayElement == nullptr) ? ResultOkAndEnded : ResultOkAndNotEnded;
                if (Ended(result))
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
            enum EMemberStructureType
            {
                STRUCTURE_TYPE_INVALID = 0,
                STRUCTURE_TYPE_BASIC   = 'B',
                STRUCTURE_TYPE_ARRAY   = 'A',
                STRUCTURE_TYPE_ENUM    = 'E'
            };

            // ----------------------------------------------------------------------------------------------------------------------------------------
            // ----------------------------------------------------------------------------------------------------------------------------------------
            // ----------------------------------------------------------------------------------------------------------------------------------------
            // Member structures
            struct member_basic_t
            {
                const char* m_name;    // member name
                i32         m_count;   // for array's, the maximum number of elements in the array
                u8          m_info[4]; // member type[2] / structure type[3]

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

                void* m_dummy;
            };
            struct member_array_t
            {
                const char* m_name;    // member name
                i32         m_size;    // -1 == unbounded, otherwise maximum size of the array
                i8          m_info[4]; // size type[1] / member type[2] / structure type[3]

                union
                {
                    void**        m_void_array;
                    bool**        m_bool_array;
                    ncore::s8**   m_i8_array;
                    ncore::s16**  m_i16_array;
                    ncore::i32**  m_i32_array;
                    ncore::i64**  m_i64_array;
                    ncore::u8**   m_u8_array;
                    ncore::u16**  m_u16_array;
                    ncore::u32**  m_u32_array;
                    ncore::u64**  m_u64_array;
                    float**       m_f32_array;
                    char**        m_char_array;
                    const char*** m_str_array;
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
                const char*           m_name;       // member name
                i32                   m_enum_count; // number of enum values
                i8                    m_info[4];    // structure type[3]
                decoder_enum_t const* m_enum;
                union
                {
                    void*       m_enum_void;
                    ncore::u8*  m_enum_u8;
                    ncore::u16* m_enum_u16;
                    ncore::u32* m_enum_u32;
                    ncore::u64* m_enum_u64;
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

                if (state->m_ObjectMember != nullptr)
                {
                    const char* name = state->m_ObjectMember->m_NamedValue->m_Name->m_String;
                    const char* end  = state->m_ObjectMember->m_NamedValue->m_Name->m_End;
                    return field_t(name, end);
                }
                return field_t(nullptr, nullptr);
            }

            bool field_equal(const field_t& field, const char* cmp_name)
            {
                ASSERT(field.is_valid());
                const char* fptr = field.m_Name;
                const char* cptr = cmp_name;
                while (fptr < field.m_End && *cptr != 0 && ascii::to_lower(*fptr) == ascii::to_lower(*cptr))
                {
                    ++fptr;
                    ++cptr;
                }
                return (fptr == field.m_End) && (*cptr == 0);
            }

            // ----------------------------------------------------------------------------------------------------------------------------------------
            // ----------------------------------------------------------------------------------------------------------------------------------------
            // ----------------------------------------------------------------------------------------------------------------------------------------
            // Basic members, single values or fixed size array's
            enum EValueType
            {
                VTYPE_NUMBER  = 3, // Parse value as number
                VTYPE_STRING  = 5, // Parse value as string
                VTYPE_MACADDR = 7, // Parse value as MAC address (6 bytes -> u64)
            };

            static void set_basic_member(decoder_t* d, const char* name, i32 size_max, u8 member_type, void* member_ptr, u8 value_type)
            {
                if (d->m_CurrentState->m_Members == nullptr)
                {
                    d->m_CurrentState->m_Members     = (member_t*)d->m_StackAllocator->CheckOut(d->m_CurrentState->m_StackAllocatorEnd);
                    d->m_CurrentState->m_MemberCount = 0;
                }
                member_t* m              = &d->m_CurrentState->m_Members[d->m_CurrentState->m_MemberCount++];
                m->m_basic.m_name        = name;
                m->m_basic.m_count       = size_max;
                m->m_basic.m_info[0]     = 0;
                m->m_basic.m_info[1]     = value_type;
                m->m_basic.m_info[2]     = member_type;
                m->m_basic.m_info[3]     = STRUCTURE_TYPE_BASIC;
                m->m_basic.m_member_void = member_ptr;
                m->m_basic.m_dummy       = nullptr;
            }

            // clang-format off
            system_type_t::system_type_t(bool* out_value) : m_bool(out_value), m_type(TYPE_BOOL), m_value_type(VTYPE_NUMBER) {}
            system_type_t::system_type_t(u8* out_value)  : m_u8(out_value), m_type(TYPE_U8), m_value_type(VTYPE_NUMBER) {}
            system_type_t::system_type_t(u16* out_value)  : m_u16(out_value), m_type(TYPE_U16), m_value_type(VTYPE_NUMBER) {}
            system_type_t::system_type_t(u32* out_value)  : m_u32(out_value), m_type(TYPE_U32), m_value_type(VTYPE_NUMBER) {}
            system_type_t::system_type_t(u64* out_value)  : m_u64(out_value), m_type(TYPE_U64), m_value_type(VTYPE_NUMBER) {}
            system_type_t::system_type_t(i8* out_value)  : m_i8(out_value), m_type(TYPE_I8), m_value_type(VTYPE_NUMBER) {}
            system_type_t::system_type_t(i16* out_value)  : m_i16(out_value), m_type(TYPE_I16), m_value_type(VTYPE_NUMBER) {}
            system_type_t::system_type_t(i32* out_value)  : m_i32(out_value), m_type(TYPE_I32), m_value_type(VTYPE_NUMBER) {}
            system_type_t::system_type_t(i64* out_value)  : m_i64(out_value), m_type(TYPE_I64), m_value_type(VTYPE_NUMBER) {}
            system_type_t::system_type_t(f32* out_value)  : m_f32(out_value), m_type(TYPE_F32), m_value_type(VTYPE_NUMBER) {}
            system_type_t::system_type_t(char* out_value)  : m_char(out_value), m_type(TYPE_CHAR), m_value_type(VTYPE_STRING) {}
            system_type_t::system_type_t(const char** out_value)  : m_str(out_value), m_type(TYPE_STRING), m_value_type(VTYPE_STRING) {}
            // clang-format on

            void register_member(decoder_t* d, const char* name, system_type_t type) { set_basic_member(d, name, 0, type.m_type, type.m_void, type.m_value_type); }

            void register_mac_addr(decoder_t* d, const char* name, system_type_t type)
            {
                ASSERT(type.m_type == TYPE_U64);
                set_basic_member(d, name, 0, type.m_type, type.m_void, VTYPE_MACADDR);
            }

            // clang-format off
            carray_type_t::carray_type_t(bool* out_value, i32 out_value_len) : m_bool(out_value), m_type(TYPE_BOOL), m_value_type(VTYPE_NUMBER) {}
            carray_type_t::carray_type_t(u8* out_value, i32 out_value_len)  : m_u8(out_value), m_type(TYPE_U8), m_value_type(VTYPE_NUMBER) {}
            carray_type_t::carray_type_t(u16* out_value, i32 out_value_len)  : m_u16(out_value), m_type(TYPE_U16), m_value_type(VTYPE_NUMBER) {}
            carray_type_t::carray_type_t(u32* out_value, i32 out_value_len)  : m_u32(out_value), m_type(TYPE_U32), m_value_type(VTYPE_NUMBER) {}
            carray_type_t::carray_type_t(u64* out_value, i32 out_value_len)  : m_u64(out_value), m_type(TYPE_U64), m_value_type(VTYPE_NUMBER) {}
            carray_type_t::carray_type_t(i8* out_value, i32 out_value_len)  : m_i8(out_value), m_type(TYPE_I8), m_value_type(VTYPE_NUMBER) {}
            carray_type_t::carray_type_t(i16* out_value, i32 out_value_len)  : m_i16(out_value), m_type(TYPE_I16), m_value_type(VTYPE_NUMBER) {}
            carray_type_t::carray_type_t(i32* out_value, i32 out_value_len)  : m_i32(out_value), m_type(TYPE_I32), m_value_type(VTYPE_NUMBER) {}
            carray_type_t::carray_type_t(i64* out_value, i32 out_value_len)  : m_i64(out_value), m_type(TYPE_I64), m_value_type(VTYPE_NUMBER) {}
            carray_type_t::carray_type_t(f32* out_value, i32 out_value_len)  : m_f32(out_value), m_type(TYPE_F32), m_value_type(VTYPE_NUMBER) {}
            carray_type_t::carray_type_t(char* out_value, i32 out_value_len)  : m_char(out_value), m_type(TYPE_CHAR), m_value_type(VTYPE_STRING) {}
            carray_type_t::carray_type_t(const char** out_value, i32 out_value_len)  : m_str(out_value), m_type(TYPE_STRING), m_value_type(VTYPE_STRING) {}
            // clang-format on

            void register_member(decoder_t* d, const char* name, carray_type_t type) { set_basic_member(d, name, type.m_maxlen, type.m_type, type.m_void, type.m_value_type); }

            // clang-format off
            array_type_t::array_type_t(bool** out_value, i32 out_value_maxlen) : m_bool(out_value), m_type(TYPE_BOOL), m_maxlen(out_value_maxlen) {}
            array_type_t::array_type_t(u8** out_value, i32 out_value_maxlen)  : m_u8(out_value), m_type(TYPE_U8), m_maxlen(out_value_maxlen) {}
            array_type_t::array_type_t(u16** out_value, i32 out_value_maxlen)  : m_u16(out_value), m_type(TYPE_U16), m_maxlen(out_value_maxlen) {}
            array_type_t::array_type_t(u32** out_value, i32 out_value_maxlen)  : m_u32(out_value), m_type(TYPE_U32), m_maxlen(out_value_maxlen) {}
            array_type_t::array_type_t(u64** out_value, i32 out_value_maxlen)  : m_u64(out_value), m_type(TYPE_U64), m_maxlen(out_value_maxlen) {}
            array_type_t::array_type_t(i8** out_value, i32 out_value_maxlen)  : m_i8(out_value), m_type(TYPE_I8), m_maxlen(out_value_maxlen) {}
            array_type_t::array_type_t(i16** out_value, i32 out_value_maxlen)  : m_i16(out_value), m_type(TYPE_I16), m_maxlen(out_value_maxlen) {}
            array_type_t::array_type_t(i32** out_value, i32 out_value_maxlen)  : m_i32(out_value), m_type(TYPE_I32), m_maxlen(out_value_maxlen) {}
            array_type_t::array_type_t(i64** out_value, i32 out_value_maxlen)  : m_i64(out_value), m_type(TYPE_I64), m_maxlen(out_value_maxlen) {}
            array_type_t::array_type_t(f32** out_value, i32 out_value_maxlen)  : m_f32(out_value), m_type(TYPE_F32), m_maxlen(out_value_maxlen) {}
            array_type_t::array_type_t(char** out_value, i32 out_value_maxlen)  : m_char(out_value), m_type(TYPE_CHAR), m_maxlen(out_value_maxlen) {}
            array_type_t::array_type_t(const char*** out_value, i32 out_value_maxlen)  : m_str(out_value), m_type(TYPE_STRING), m_maxlen(out_value_maxlen) {}
            // clang-format on

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
                m->m_array.m_size       = size_max; // -1 == unbounded, otherwise maximum size of the array
                m->m_array.m_info[0]    = 0;
                m->m_array.m_info[1]    = size_type;
                m->m_array.m_info[2]    = member_type;
                m->m_array.m_info[3]    = STRUCTURE_TYPE_ARRAY;
                m->m_array.m_void_array = array_ptr;
                m->m_array.m_void_size  = size_ptr;
            }

            void register_member(decoder_t* d, const char* name, array_type_t arraytype, u8* out_array_size) { set_array_member(d, name, arraytype.m_maxlen, arraytype.m_type, arraytype.m_void, TYPE_U8, out_array_size); }
            void register_member(decoder_t* d, const char* name, array_type_t arraytype, u16* out_array_size) { set_array_member(d, name, arraytype.m_maxlen, arraytype.m_type, arraytype.m_void, TYPE_U16, out_array_size); }
            void register_member(decoder_t* d, const char* name, array_type_t arraytype, u32* out_array_size) { set_array_member(d, name, arraytype.m_maxlen, arraytype.m_type, arraytype.m_void, TYPE_U32, out_array_size); }
            void register_member(decoder_t* d, const char* name, array_type_t arraytype, u64* out_array_size) { set_array_member(d, name, arraytype.m_maxlen, arraytype.m_type, arraytype.m_void, TYPE_U64, out_array_size); }
            void register_member(decoder_t* d, const char* name, array_type_t arraytype, i8* out_array_size) { set_array_member(d, name, arraytype.m_maxlen, arraytype.m_type, arraytype.m_void, TYPE_I8, out_array_size); }
            void register_member(decoder_t* d, const char* name, array_type_t arraytype, i16* out_array_size) { set_array_member(d, name, arraytype.m_maxlen, arraytype.m_type, arraytype.m_void, TYPE_I16, out_array_size); }
            void register_member(decoder_t* d, const char* name, array_type_t arraytype, i32* out_array_size) { set_array_member(d, name, arraytype.m_maxlen, arraytype.m_type, arraytype.m_void, TYPE_I32, out_array_size); }
            void register_member(decoder_t* d, const char* name, array_type_t arraytype, i64* out_array_size) { set_array_member(d, name, arraytype.m_maxlen, arraytype.m_type, arraytype.m_void, TYPE_I64, out_array_size); }

            // ----------------------------------------------------------------------------------------------------------------------------------------
            // ----------------------------------------------------------------------------------------------------------------------------------------
            // ----------------------------------------------------------------------------------------------------------------------------------------
            // Enum members
            void decode_enum(decoder_t* d, member_enum_t const* e, u64& out_enum_value)
            {
                out_enum_value = 0;
                if (e->m_info[1] == 1)
                {
                    field_t flags = decode_string_as_field(d);
                    i32     enum_index;
                    while (decode_find_flag(e->m_enum->m_enum_strs, e->m_enum_count, flags, enum_index))
                    {
                        if (enum_index >= 0 && enum_index < e->m_enum_count)
                        {
                            switch (e->m_info[2])
                            {
                                case TYPE_U8: out_enum_value |= e->m_enum->m_values_u8[enum_index]; break;
                                case TYPE_U16: out_enum_value |= e->m_enum->m_values_u16[enum_index]; break;
                                case TYPE_U32: out_enum_value |= e->m_enum->m_values_u32[enum_index]; break;
                                case TYPE_U64: out_enum_value |= e->m_enum->m_values_u64[enum_index]; break;
                                default: break;
                            }
                        }
                    }
                }
                else
                {
                    const i32 enum_index = decode_find_enum(d, e->m_enum->m_enum_strs, e->m_enum_count);
                    if (enum_index >= 0 && enum_index < e->m_enum_count)
                    {
                        switch (e->m_info[2])
                        {
                            case TYPE_U8: out_enum_value = e->m_enum->m_values_u8[enum_index]; break;
                            case TYPE_U16: out_enum_value = e->m_enum->m_values_u16[enum_index]; break;
                            case TYPE_U32: out_enum_value = e->m_enum->m_values_u32[enum_index]; break;
                            case TYPE_U64: out_enum_value = e->m_enum->m_values_u64[enum_index]; break;
                            default: break;
                        }
                    }
                }
            }

            member_enum_t* register_enum(decoder_t* d, const char* name, decoder_enum_t const* de)
            {
                if (d->m_CurrentState->m_Members == nullptr)
                {
                    d->m_CurrentState->m_Members     = (member_t*)d->m_StackAllocator->CheckOut(d->m_CurrentState->m_StackAllocatorEnd);
                    d->m_CurrentState->m_MemberCount = 0;
                }
                member_t*      m  = &d->m_CurrentState->m_Members[d->m_CurrentState->m_MemberCount++];
                member_enum_t* em = &m->m_enum;
                em->m_name        = name;
                em->m_enum_count  = de->m_enum_count;
                em->m_info[0]     = 0;
                em->m_info[1]     = de->m_as_flags ? 1 : 0;
                em->m_info[2]     = TYPE_INVALID;
                em->m_info[3]     = STRUCTURE_TYPE_ENUM;
                em->m_enum        = de;
                return em;
            }

            void register_member(decoder_t* d, const char* name, decoder_enum_t const* de, u8* out_value)
            {
                member_enum_t* em = register_enum(d, name, de);
                em->m_info[2]     = TYPE_U8;
                em->m_enum_u8     = out_value;
            }
            void register_member(decoder_t* d, const char* name, decoder_enum_t const* decoder_enum, u16* out_value)
            {
                member_enum_t* em = register_enum(d, name, decoder_enum);
                em->m_info[2]     = TYPE_U16;
                em->m_enum_u16    = out_value;
            }
            void register_member(decoder_t* d, const char* name, decoder_enum_t const* decoder_enum, u32* out_value)
            {
                member_enum_t* em = register_enum(d, name, decoder_enum);
                em->m_info[2]     = TYPE_U32;
                em->m_enum_u32    = out_value;
            }
            void register_member(decoder_t* d, const char* name, decoder_enum_t const* decoder_enum, u64* out_value)
            {
                member_enum_t* em = register_enum(d, name, decoder_enum);
                em->m_info[2]     = TYPE_U64;
                em->m_enum_u64    = out_value;
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
                    member_t* m = &state->m_Members[i];
                    if (field_equal(field, m->m_basic.m_name))
                    {
                        member = m;
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
                            if (member->m_basic.m_count == 0)
                            {
                                if (member->m_basic.m_info[1] == VTYPE_NUMBER)
                                {
                                    // single value
                                    switch (member->m_basic.m_info[2])
                                    {
                                        case TYPE_BOOL: *member->m_basic.m_member_bool = decode_bool(d); break;
                                        case TYPE_I8: *member->m_basic.m_member_i8 = decode_i8(d); break;
                                        case TYPE_I16: *member->m_basic.m_member_i16 = decode_i16(d); break;
                                        case TYPE_I32: *member->m_basic.m_member_i32 = decode_i32(d); break;
                                        case TYPE_I64: *member->m_basic.m_member_i64 = decode_i64(d); break;
                                        case TYPE_U8: *member->m_basic.m_member_u8 = decode_u8(d); break;
                                        case TYPE_U16: *member->m_basic.m_member_u16 = decode_u16(d); break;
                                        case TYPE_U32: *member->m_basic.m_member_u32 = decode_u32(d); break;
                                        case TYPE_U64: *member->m_basic.m_member_u64 = decode_u64(d); break;
                                        case TYPE_F32: *member->m_basic.m_member_f32 = decode_f32(d); break;
                                    }
                                }
                                else if (member->m_basic.m_info[1] == VTYPE_STRING)
                                {
                                    // single value
                                    switch (member->m_basic.m_info[2])
                                    {
                                        case TYPE_CHAR: *member->m_basic.m_member_char = decode_char(d); break;
                                        case TYPE_STRING: *member->m_basic.m_member_string = decode_string(d); break;
                                        default: return false;
                                    }
                                }
                                else if (member->m_basic.m_info[1] == VTYPE_MACADDR)
                                {
                                    // single MAC address (6 bytes stored in u64)
                                    u64 mac_addr                  = decode_mac_addr(d);
                                    *member->m_basic.m_member_u64 = mac_addr;
                                }
                                return true;
                            }
                            else
                            {
                                // c-style array
                                switch (member->m_basic.m_info[2])
                                {
                                    case TYPE_BOOL: decode_carray_bool(d, member->m_basic.m_member_bool, member->m_basic.m_count); break;
                                    case TYPE_I8: decode_carray_i8(d, member->m_basic.m_member_i8, member->m_basic.m_count); break;
                                    case TYPE_I16: decode_carray_i16(d, member->m_basic.m_member_i16, member->m_basic.m_count); break;
                                    case TYPE_I32: decode_carray_i32(d, member->m_basic.m_member_i32, member->m_basic.m_count); break;
                                    case TYPE_I64: decode_carray_i64(d, member->m_basic.m_member_i64, member->m_basic.m_count); break;
                                    case TYPE_U8: decode_carray_u8(d, member->m_basic.m_member_u8, member->m_basic.m_count); break;
                                    case TYPE_U16: decode_carray_u16(d, member->m_basic.m_member_u16, member->m_basic.m_count); break;
                                    case TYPE_U32: decode_carray_u32(d, member->m_basic.m_member_u32, member->m_basic.m_count); break;
                                    case TYPE_U64: decode_carray_u64(d, member->m_basic.m_member_u64, member->m_basic.m_count); break;
                                    case TYPE_F32: decode_carray_f32(d, member->m_basic.m_member_f32, member->m_basic.m_count); break;
                                    case TYPE_CHAR: decode_carray_char(d, member->m_basic.m_member_char, member->m_basic.m_count); break;
                                    case TYPE_STRING: decode_carray_str(d, member->m_basic.m_member_string, member->m_basic.m_count); break;
                                    default: return false;
                                }
                            }
                            break;
                        }
                        case STRUCTURE_TYPE_ARRAY:
                        {
                            i32 out_array_size = 0;
                            switch (member->m_array.m_info[2]) // member type
                            {
                                case TYPE_BOOL_ARRAY: decode_array_bool(d, *member->m_array.m_bool_array, out_array_size, member->m_array.m_size); break;
                                case TYPE_I8_ARRAY: decode_array_i8(d, *member->m_array.m_i8_array, out_array_size, member->m_array.m_size); break;
                                case TYPE_I16_ARRAY: decode_array_i16(d, *member->m_array.m_i16_array, out_array_size, member->m_array.m_size); break;
                                case TYPE_I32_ARRAY: decode_array_i32(d, *member->m_array.m_i32_array, out_array_size, member->m_array.m_size); break;
                                case TYPE_I64_ARRAY: decode_array_i64(d, *member->m_array.m_i64_array, out_array_size, member->m_array.m_size); break;
                                case TYPE_U8_ARRAY: decode_array_u8(d, *member->m_array.m_u8_array, out_array_size, member->m_array.m_size); break;
                                case TYPE_U16_ARRAY: decode_array_u16(d, *member->m_array.m_u16_array, out_array_size, member->m_array.m_size); break;
                                case TYPE_U32_ARRAY: decode_array_u32(d, *member->m_array.m_u32_array, out_array_size, member->m_array.m_size); break;
                                case TYPE_U64_ARRAY: decode_array_u64(d, *member->m_array.m_u64_array, out_array_size, member->m_array.m_size); break;
                                case TYPE_F32_ARRAY: decode_array_f32(d, *member->m_array.m_f32_array, out_array_size, member->m_array.m_size); break;
                                case TYPE_CHAR_ARRAY: decode_array_char(d, *member->m_array.m_char_array, out_array_size, member->m_array.m_size); break;
                                case TYPE_STRING_ARRAY: decode_array_str(d, *member->m_array.m_str_array, out_array_size, member->m_array.m_size); break;
                                default: return false;
                            }

                            switch (member->m_array.m_info[1]) // size type
                            {
                                case TYPE_I8: *member->m_array.m_i8_size = (i8)out_array_size; break;
                                case TYPE_I16: *member->m_array.m_i16_size = (i16)out_array_size; break;
                                case TYPE_I32: *member->m_array.m_i32_size = (i32)out_array_size; break;
                                case TYPE_I64: *member->m_array.m_i64_size = (i64)out_array_size; break;
                                case TYPE_U8: *member->m_array.m_u8_size = (u8)out_array_size; break;
                                case TYPE_U16: *member->m_array.m_u16_size = (u16)out_array_size; break;
                                case TYPE_U32: *member->m_array.m_u32_size = (u32)out_array_size; break;
                                case TYPE_U64: *member->m_array.m_u64_size = (u64)out_array_size; break;
                                default: return false;
                            }

                            break;
                        }
                        case STRUCTURE_TYPE_ENUM:
                        {
                            u64 enum_value = 0;
                            decode_enum(d, &member->m_enum, enum_value);
                            switch (member->m_enum.m_info[2]) // member type
                            {
                                case TYPE_U8: *member->m_enum.m_enum_u8 = (u8)enum_value; break;
                                case TYPE_U16: *member->m_enum.m_enum_u16 = (u16)enum_value; break;
                                case TYPE_U32: *member->m_enum.m_enum_u32 = (u32)enum_value; break;
                                case TYPE_U64: *member->m_enum.m_enum_u64 = (u64)enum_value; break;
                                default: return false;
                            }
                            return true;
                        }
                    }
                }
                return false;
            }

        } // namespace ndecoder
    } // namespace njson
} // namespace ncore

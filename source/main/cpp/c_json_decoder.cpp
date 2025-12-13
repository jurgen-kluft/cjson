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
            struct state_t
            {
                nscanner::JsonValue const*            m_Value;
                i32                                   m_Index; // Current index in iteration
                i32                                   m_Size;  // Total size of members/elements
                nscanner::JsonLinkedNamedValue const* m_CurrentMember;
                nscanner::JsonLinkedValue const*      m_CurrentElement;

                void reset(nscanner::JsonValue const* value)
                {
                    m_Value          = value;
                    m_Index          = -1;
                    m_Size           = -1;
                    m_CurrentMember  = nullptr;
                    m_CurrentElement = nullptr;
                }
            };

            struct state_array_t
            {
                i32       m_Capacity;
                i32       m_Size;
                state_t** m_States;

                void init(JsonAllocator* allocator, i32 capacity)
                {
                    m_Capacity = capacity;
                    m_Size     = 0;
                    m_States   = allocator->AllocateArray<state_t*>(capacity);
                }

                inline bool is_empty() const { return m_Size == 0; }

                state_t* pop()
                {
                    if (m_Size > 0)
                    {
                        m_Size -= 1;
                        return m_States[m_Size];
                    }
                    return nullptr;
                }

                bool push(state_t* state)
                {
                    if (m_Size < m_Capacity)
                    {
                        m_States[m_Size] = state;
                        m_Size += 1;
                        return true;
                    }
                    return false;
                }
            };

            decoder_t* create_decoder(JsonAllocator* scratch_allocator, JsonAllocator* decoder_allocator, const char* json, const char* json_end)
            {
                const char*                errmsg = nullptr;
                nscanner::JsonValue const* root   = nscanner::Scan(json, json_end, scratch_allocator, errmsg);
                if (errmsg != nullptr)
                    return nullptr;

                const i32 stack_capacity = 64;

                decoder_t* d          = scratch_allocator->Allocate<decoder_t>();
                d->m_ScratchAllocator = scratch_allocator;
                d->m_DecoderAllocator = decoder_allocator;
                d->m_ReusableStates   = scratch_allocator->Allocate<state_array_t>();
                d->m_ReusableStates->init(scratch_allocator, stack_capacity);
                d->m_Stack = scratch_allocator->Allocate<state_array_t>();
                d->m_Stack->init(scratch_allocator, stack_capacity);

                d->m_Current = d->m_ReusableStates->pop();
                d->m_Current->reset(root);

                d->m_Stack->push(d->m_Current);
                d->m_Current = nullptr;

                result_t result = read_until_object_end(d);
                if (!result.ok || result.end)
                {
                    destroy_decoder(d);
                    return nullptr;
                }
                return d;
            }

            void destroy_decoder(decoder_t*& d) { d->m_ScratchAllocator->Reset(); }

            void decode_bool(decoder_t* d, bool& out_value)
            {
                const nscanner::JsonNumberValue* number = nullptr;
                state_t*                          state      = d->m_Current;
                if (state->m_CurrentMember != nullptr && state->m_CurrentMember->m_NamedValue->m_Value->IsBoolean())
                {
                    number = state->m_CurrentMember->m_NamedValue->m_Value->AsNumber();
                }
                else if (state->m_CurrentElement != nullptr && state->m_CurrentElement->m_Value->IsBoolean())
                {
                    number = state->m_CurrentElement->m_Value->AsNumber();
                }
                if (number != nullptr)
                {
                    out_value = ParseBoolean(number->m_String, number->m_End);
                }
                else
                {
                    out_value = false;
                }
            }

            void decode_i16(decoder_t* d, i16& out_value)
            {
                const nscanner::JsonNumberValue* number = nullptr;
                state_t*                         state  = d->m_Current;
                if (state->m_CurrentMember != nullptr && state->m_CurrentMember->m_NamedValue->m_Value->IsNumber())
                {
                    number = state->m_CurrentMember->m_NamedValue->m_Value->AsNumber();
                }
                else if (state->m_CurrentElement != nullptr && state->m_CurrentElement->m_Value->IsNumber())
                {
                    number = state->m_CurrentElement->m_Value->AsNumber();
                }
                JsonNumber json_number;
                if (number != nullptr)
                {
                    const char* str = number->m_String;
                    ParseNumber(str, number->m_End, json_number);
                }
                out_value = (i16)JsonNumberAsInt64(json_number);
            }

            void decode_i32(decoder_t* d, i32& out_value)
            {
                const nscanner::JsonNumberValue* number = nullptr;
                state_t*                         state  = d->m_Current;
                if (state->m_CurrentMember != nullptr && state->m_CurrentMember->m_NamedValue->m_Value->IsNumber())
                {
                    number = state->m_CurrentMember->m_NamedValue->m_Value->AsNumber();
                }
                else if (state->m_CurrentElement != nullptr && state->m_CurrentElement->m_Value->IsNumber())
                {
                    number = state->m_CurrentElement->m_Value->AsNumber();
                }
                JsonNumber json_number;
                if (number != nullptr)
                {
                    const char* str = number->m_String;
                    ParseNumber(str, number->m_End, json_number);
                }
                out_value = (i32)JsonNumberAsInt64(json_number);
            }

            void decode_f32(decoder_t* d, f32& out_value)
            {
                const nscanner::JsonNumberValue* number = nullptr;
                state_t*                         state  = d->m_Current;
                if (state->m_CurrentMember != nullptr && state->m_CurrentMember->m_NamedValue->m_Value->IsNumber())
                {
                    number = state->m_CurrentMember->m_NamedValue->m_Value->AsNumber();
                }
                else if (state->m_CurrentElement != nullptr && state->m_CurrentElement->m_Value->IsNumber())
                {
                    number = state->m_CurrentElement->m_Value->AsNumber();
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
                state_t*                         state     = d->m_Current;
                if (state->m_CurrentMember != nullptr && state->m_CurrentMember->m_NamedValue->m_Value->IsString())
                {
                    str_value = state->m_CurrentMember->m_NamedValue->m_Value->AsString();
                }
                else if (state->m_CurrentElement != nullptr && state->m_CurrentElement->m_Value->IsString())
                {
                    str_value = state->m_CurrentElement->m_Value->AsString();
                }
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
                state_t*                         state     = d->m_Current;
                if (state->m_CurrentMember != nullptr && state->m_CurrentMember->m_NamedValue->m_Value->IsString())
                {
                    str_value = state->m_CurrentMember->m_NamedValue->m_Value->AsString();
                }
                else if (state->m_CurrentElement != nullptr && state->m_CurrentElement->m_Value->IsString())
                {
                    str_value = state->m_CurrentElement->m_Value->AsString();
                }
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

            void decode_array_f32(decoder_t* d, f32*& out_array, i32& out_array_size, i32 out_array_maxsize)
            {
                i32      array_size;
                result_t result = read_until_array_end(d, array_size);
                array_size      = (array_size > (i32)out_array_maxsize) ? (i32)out_array_maxsize : array_size;
                out_array_size  = (u32)array_size;
                out_array       = d->m_DecoderAllocator->AllocateArray<f32>(out_array_size);
                i32 array_index = 0;
                while (result.ok && !result.end)
                {
                    float color_component;
                    decode_f32(d, color_component);
                    if (array_index < (i32)out_array_size)
                        out_array[array_index] = color_component;
                    array_index++;
                    result = read_until_array_end(d, array_size);
                }
            }

            void decode_carray_f32(decoder_t* d, f32* out_array, i32 out_array_maxlen)
            {
                i32      array_size;
                result_t result      = read_until_array_end(d, array_size);
                i32      array_index = 0;
                while (result.ok && !result.end)
                {
                    float color_component;
                    decode_f32(d, color_component);
                    if (array_index < (i32)out_array_maxlen)
                        out_array[array_index] = color_component;
                    array_index++;
                    result = read_until_array_end(d, array_size);
                }
            }

            void decode_enum(decoder_t* d, u64& out_enum_value, const char** enum_strs, const u64* enum_values, i32 enum_count)
            {
                // TODO
                // Not implemented yet
            }


            field_t decode_field(decoder_t* d)
            {
                state_t* state = d->m_Current;
                field_t  field;
                if (state->m_CurrentMember != nullptr)
                {
                    field.m_Name = state->m_CurrentMember->m_NamedValue->m_Name->m_String;
                    field.m_End  = state->m_CurrentMember->m_NamedValue->m_Name->m_End;
                }
                else
                {
                    field.m_Name = nullptr;
                    field.m_End  = nullptr;
                }
                return field;
            }

            result_t read_until_object_end(decoder_t* d)
            {
                if (d->m_Current == nullptr)
                {
                    if (d->m_Stack->is_empty())
                    {
                        return result_t(false, true);
                    }
                    d->m_Current = d->m_Stack->pop();
                }

                if (!d->m_Current->m_Value->IsObject())
                    return result_t(false, true);

                if (d->m_Current->m_Index == -1)
                {
                    d->m_Current->m_Index         = 0;
                    d->m_Current->m_Size          = d->m_Current->m_Value->AsObject()->m_Count;
                    d->m_Current->m_CurrentMember = d->m_Current->m_Value->AsObject()->m_LinkedList;
                }
                else
                {
                    d->m_Current->m_CurrentMember = d->m_Current->m_CurrentMember->m_Next;
                    d->m_Current->m_Index += 1;
                }

                // Return whether we are at the end of this array
                const result_t current_result = result_t(true, (d->m_Current->m_CurrentMember == nullptr));

                // If the current array element is an object or an array, push current on the stack
                // and make the current object member or array element the new current.
                if (d->m_Current->m_CurrentMember != nullptr)
                {
                    const nscanner::JsonValue* elem_value = d->m_Current->m_CurrentMember->m_NamedValue->m_Value;
                    if (elem_value->IsObject() || elem_value->IsArray())
                    {
                        state_t* new_state = d->m_ReusableStates->pop();
                        if (new_state == nullptr)
                        {
                            new_state = d->m_DecoderAllocator->Allocate<state_t>();
                        }
                        new_state->reset(elem_value);
                        d->m_Stack->push(d->m_Current);
                        d->m_Current = new_state;
                    }
                }

                return current_result;
            }

            result_t read_until_array_end(decoder_t* d)
            {
                i32 dummy_size = 0;
                return read_until_array_end(d, dummy_size);
            }

            result_t read_until_array_end(decoder_t* d, i32& out_size)
            {
                if (d->m_Current == nullptr)
                {
                    if (d->m_Stack->is_empty())
                    {
                        return result_t(false, true);
                    }
                    d->m_Current = d->m_Stack->pop();
                }

                if (!d->m_Current->m_Value->IsArray())
                    return result_t(false, true);

                if (d->m_Current->m_Index == -1)
                {
                    d->m_Current->m_Index          = 0;
                    d->m_Current->m_Size           = d->m_Current->m_Value->AsArray()->m_Count;
                    d->m_Current->m_CurrentElement = d->m_Current->m_Value->AsArray()->m_LinkedList;
                }
                else
                {
                    d->m_Current->m_CurrentElement = d->m_Current->m_CurrentElement->m_Next;
                    d->m_Current->m_Index += 1;
                }

                // Return whether we are at the end of this array
                const result_t current_result = result_t(true, (d->m_Current->m_CurrentElement == nullptr));
                out_size                      = d->m_Current->m_Size;

                // If the current array element is an object or an array, push current on the stack
                // and make the current object member or array element the new current.
                if (d->m_Current->m_CurrentElement != nullptr)
                {
                    const nscanner::JsonValue* elem_value = d->m_Current->m_CurrentElement->m_Value;
                    if (elem_value->IsObject() || elem_value->IsArray())
                    {
                        state_t* new_state = d->m_ReusableStates->pop();
                        if (new_state == nullptr)
                        {
                            new_state = d->m_DecoderAllocator->Allocate<state_t>();
                        }
                        new_state->reset(elem_value);
                        d->m_Stack->push(d->m_Current);
                        d->m_Current = new_state;
                    }
                }

                return current_result;
            }

        } // namespace ndecoder
    } // namespace njson

} // namespace ncore

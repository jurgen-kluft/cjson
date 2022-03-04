#include "xbase/x_allocator.h"
#include "xbase/x_context.h"
#include "xbase/x_printf.h"
#include "xbase/x_runes.h"
#include "xjson/x_json.h"
#include "xjson/x_json_utils.h"
#include "xjson/x_json_encode.h"
#include "xjson/x_json_decode.h"

namespace xcore
{
    namespace json
    {
        static bool member_as_bool(const JsonMember& member)
        {
            if (member.is_pointer())
                return *(*(bool**)member.m_data_ptr);
            return *(bool*)member.m_data_ptr;
        }

        static s64 member_as_int(const JsonMember& member)
        {
            void* p = member.m_data_ptr;
            if (member.is_pointer())
                p = *((void**)p);

            if (member.is_int8())
            {
                return (s64)(*(s8*)p);
            }
            else if (member.is_int16())
            {
                return (s64)(*(s16*)p);
            }
            else if (member.is_int32())
            {
                return (s64)(*(s32*)p);
            }
            else if (member.is_int64())
            {
                return (s64)(*(s64*)p);
            }
            return 0;
        }

        static u64 member_as_uint(const JsonMember& member)
        {
            void* p = member.m_data_ptr;
            if (member.is_pointer())
                p = *((void**)p);

            if (member.is_uint8())
            {
                return (u64)(*(u8*)p);
            }
            else if (member.is_uint16())
            {
                return (u64)(*(u16*)p);
            }
            else if (member.is_uint32())
            {
                return (u64)(*(u32*)p);
            }
            else if (member.is_uint64())
            {
                return (u64)(*(u64*)p);
            }
            return 0;
        }

        static f32 member_as_f32(const JsonMember& member)
        {
            void* p = member.m_data_ptr;
            if (member.is_pointer())
                p = *((void**)p);
            return (f32)(*(f32*)p);
        }

        static f64 member_as_f64(const JsonMember& member)
        {
            void* p = member.m_data_ptr;
            if (member.is_pointer())
                p = *((void**)p);
            return (f64)(*(f64*)p);
        }

        static const char* member_as_string(const JsonMember& member)
        {
            ASSERT(member.is_string() && !member.is_pointer());
            return *((const char**)member.m_data_ptr);
        }

        struct jsondoc_t
        {
            char const* m_json_text_begin;
            char*       m_json_text;
            char const* m_json_text_end;
            s32         m_indent;

            inline void writeString(char const* str)
            {
                while (*str && m_json_text < m_json_text_end)
                {
                    *m_json_text++ = *str++;
                }
            }

            void writeIndent()
            {
                for (s32 i = 0; i < m_indent; ++i)
                {
                    writeString("  ");
                }
            }

            void startObject()
            {
                writeString("{");
                writeString("\n");
                ++m_indent;
            }

            void endObject()
            {
                --m_indent;
                writeIndent();
                writeString("}");
            }

            void startArray()
            {
                writeString("[");
                writeString("\n");
                ++m_indent;
            }

            void endArray()
            {
                --m_indent;
                writeIndent();
                writeString("]");
            }

            void writeValueString(const char* str)
            {
                writeString("\"");
                writeString(str);
                writeString("\"");
            }
            void writeValueBool(bool value) { writeString(value ? "true" : "false"); }
            void writeValueInt64(s64 field_value) { m_json_text = itoa(field_value, m_json_text, m_json_text_end, 10); }
            void writeValueUInt64(u64 field_value) { m_json_text = utoa(field_value, m_json_text, m_json_text_end, 10); }
            void writeValueFloat(f32 field_value) { m_json_text = ftoa(field_value, m_json_text, m_json_text_end); }
            void writeValueDouble(f64 field_value) { m_json_text = dtoa(field_value, m_json_text, m_json_text_end); }

            void startField(const char* field_name)
            {
                writeIndent();
                writeString("\"");
                writeString(field_name);
                writeString("\": ");
            }

            void end(bool is_last)
            {
                if (!is_last)
                {
                    writeString(",\n");
                }
                else
                {
                    writeString("\n");
                }
            }
        };

        static bool JsonEncodeValue(JsonMember& member, jsondoc_t& doc, char const*& error_message);

        static bool JsonEncodeArray(JsonObject& object, JsonMember& member, jsondoc_t& doc, char const*& error_message)
        {
            char* array_ptr;
            s32   array_size;
            if (member.is_array())
            {
                array_size = member.m_descr->m_csize;
                array_ptr  = (char*)member.m_data_ptr;
            }
            else if (member.is_array_ptr())
            {
                if (member.is_array_ptr_size8())
                {
                    uptr const offset = (uptr)member.m_descr->m_size8 - (uptr)object.m_descr->m_default;
                    s8*        size8  = (s8*)((uptr)object.m_instance + offset);
                    array_size        = *size8;
                }
                else if (member.is_array_ptr_size16())
                {
                    uptr const offset = (uptr)member.m_descr->m_size16 - (uptr)object.m_descr->m_default;
                    s16*       size16 = (s16*)((uptr)object.m_instance + offset);
                    array_size        = *size16;
                }
                else if (member.is_array_ptr_size32())
                {
                    uptr const offset = (uptr)member.m_descr->m_size32 - (uptr)object.m_descr->m_default;
                    s32*       size32 = (s32*)((uptr)object.m_instance + offset);
                    array_size        = *size32;
                }
                array_ptr = *((char**)member.m_data_ptr);
            }

            if (array_size > 0)
            {
                doc.startArray();
                u32 const aligned_size = (member.m_descr->m_typedescr->m_sizeof + (member.m_descr->m_typedescr->m_alignof - 1)) & ~(member.m_descr->m_typedescr->m_alignof - 1);
                for (s32 i = 0; i < array_size; ++i)
                {
                    JsonMember e;
                    e.m_descr    = member.m_descr;
                    e.m_data_ptr = array_ptr + (i * aligned_size);
                    doc.writeIndent();
                    JsonEncodeValue(e, doc, error_message);
                    doc.end(i == (array_size - 1));
                }
                doc.endArray();
            }

            return true;
        }

        static bool JsonEncodeObject(JsonObject& object, jsondoc_t& doc, char const*& error_message)
        {
            doc.startObject();

            s32 const n = object.m_descr->m_member_count;
            for (s32 i = 0; i < n; ++i)
            {
                JsonFieldDescr const* member_descr = object.m_descr->m_members + i;
                JsonMember            member;
                member.m_descr    = member_descr;
                member.m_data_ptr = member.get_member_ptr(object);

                doc.startField(member.m_descr->m_name);

                if (member.is_array() || member.is_array_ptr())
                {
                    if (!JsonEncodeArray(object, member, doc, error_message))
                    {
                        return false;
                    }
                }
                else
                {
                    JsonEncodeValue(member, doc, error_message);
                }

                doc.end(i == (n - 1));
            }

            doc.endObject();
            return true;
        }

        static bool JsonEncodeValue(JsonMember& member, jsondoc_t& doc, char const*& error_message)
        {
            if (member.is_bool())
            {
                doc.writeValueBool(member_as_bool(member));
            }
            else if (member.is_number())
            {
                if (member.is_f32())
                {
                    doc.writeValueFloat(member_as_f32(member));
                }
                else if (member.is_f64())
                {
                    doc.writeValueDouble(member_as_f64(member));
                }
                else if (member.is_int())
                {
                    doc.writeValueInt64(member_as_int(member));
                }
                else if (member.is_uint())
                {
                    doc.writeValueUInt64(member_as_uint(member));
                }
            }
            else if (member.is_string())
            {
                doc.writeValueString(member_as_string(member));
            }
            else if (member.is_object())
            {
                JsonObject o;
                o.m_descr = member.m_descr->m_typedescr;
                if (member.is_pointer())
                {
                    o.m_instance = *((void**)member.m_data_ptr);
                }
                else
                {
                    o.m_instance = ((void*)member.m_data_ptr);
                }
                if (!JsonEncodeObject(o, doc, error_message))
                {
                    return false;
                }
            }

            return true;
        }

        bool JsonEncode(JsonObject& root_object, char* json_text, char const* json_text_end, char const*& error_message)
        {
            jsondoc_t doc;
            doc.m_json_text_begin = json_text;
            doc.m_json_text       = json_text;
            doc.m_json_text_end   = json_text_end;
            doc.m_indent          = 0;

            if (!JsonEncodeObject(root_object, doc, error_message))
            {
                return false;
            }
            return true;
        }

    } // namespace json

} // namespace xcore
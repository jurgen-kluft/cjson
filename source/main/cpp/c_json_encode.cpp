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

            switch (member.all_int())
            {
                case JsonType::TypeUInt8: return (s64)(*(s8*)p);
                case JsonType::TypeUInt16: return (s64)(*(s16*)p);
                case JsonType::TypeUInt32: return (s64)(*(s32*)p);
                case JsonType::TypeUInt64: return (s64)(*(s64*)p);
                default: break;
            }
            return 0;
        }

        static u64 member_as_uint(const JsonMember& member)
        {
            void* p = member.m_data_ptr;
            if (member.is_pointer())
                p = *((void**)p);

            switch (member.all_int())
            {
                case JsonType::TypeUInt8: return (u64)(*(u8*)p);
                case JsonType::TypeUInt16: return (u64)(*(u16*)p);
                case JsonType::TypeUInt32: return (u64)(*(u32*)p);
                case JsonType::TypeUInt64: return (u64)(*(u64*)p);
                default: break;
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

        static u64 member_as_enum(const JsonMember& member)
        {
            ASSERT(member.is_enum() && !member.is_pointer());
            if (member.is_enum16())
                return (u64)(*(u16*)member.m_data_ptr);
            else if (member.is_enum32())
                return (u64)(*(u32*)member.m_data_ptr);
            else if (member.is_enum64())
                return (u64)(*(u64*)member.m_data_ptr);

            return 0;
        }

        struct jsondoc_t
        {
            char const* m_json_text_begin;
            char*       m_json_text;
            char const* m_json_text_end;
            s32         m_indent;
            s32         m_indent_spaces;
            char        m_indent_str[64 + 1];
            jsondoc_t()
            {
                m_indent_spaces = 2;
                for (s32 i = 0; i < sizeof(m_indent_str); ++i)
                    m_indent_str[i] = ' ';
                m_indent_str[sizeof(m_indent_str) - 1] = '\0';
            }

            inline void writeString(char const* str)
            {
                while (m_json_text < m_json_text_end && *str != 0)
                {
                    *m_json_text++ = *str++;
                }
            }

            void writeIndent()
            {
                s32 n = m_indent >> 6;
                while (n > 0)
                {
                    writeString(m_indent_str);
                    n -= 1;
                }
                writeString(m_indent_str + 64 - (m_indent & 0x3F));
            }

            void startObject()
            {
                writeString("{\n");
                m_indent += m_indent_spaces;
            }

            void endObject()
            {
                m_indent -= m_indent_spaces;
                writeIndent();
                writeString("}");
                *m_json_text = '\0';
            }

            void startArray()
            {
                writeString("[\n");
                m_indent += m_indent_spaces;
            }

            void endArray()
            {
                m_indent -= m_indent_spaces;
                writeIndent();
                writeString("]");
                *m_json_text = '\0';
            }

            void writeValueString(const char* str)
            {
                writeString("\"");
                writeString(str);
                writeString("\"");
            }

            void writeValueEnum(JsonMember& member)
            {
                writeString("\"");
                ASSERT(member.is_enum() && !member.is_pointer());
                JsonEnumTypeDef* enum_type = member.m_descr->m_typedescr->as_enum_type();
                if (enum_type != nullptr)
                {
                    u64 const    eval  = member_as_enum(member);
                    const char** estrs = enum_type->m_enum_strs;
                    if (enum_type->m_to_str != nullptr)
                    {
                        enum_type->m_to_str(eval, estrs, m_json_text, m_json_text_end);
                    }
                    else
                    {
                        EnumToString(eval, estrs, m_json_text, m_json_text_end);
                    }
                }

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
            char* array_ptr  = nullptr;
            s32   array_size = 0;
            if (member.is_array())
            {
                array_size = member.m_descr->m_csize;
                array_ptr  = (char*)member.m_data_ptr;
            }
            else if (member.is_array_ptr())
            {
                JsonObjectTypeDef const* obj_type_def = object.m_descr->as_object_type();

                if (member.is_array_ptr_size8())
                {
                    uptr const offset = (uptr)member.m_descr->m_size8 - (uptr)obj_type_def->m_default;
                    s8*        size8  = (s8*)((uptr)object.m_instance + offset);
                    array_size        = *size8;
                }
                else if (member.is_array_ptr_size16())
                {
                    uptr const offset = (uptr)member.m_descr->m_size16 - (uptr)obj_type_def->m_default;
                    s16*       size16 = (s16*)((uptr)object.m_instance + offset);
                    array_size        = *size16;
                }
                else if (member.is_array_ptr_size32())
                {
                    uptr const offset = (uptr)member.m_descr->m_size32 - (uptr)obj_type_def->m_default;
                    s32*       size32 = (s32*)((uptr)object.m_instance + offset);
                    array_size        = *size32;
                }
                array_ptr = *((char**)member.m_data_ptr);
            }

            doc.startArray();
            if (array_size > 0 && array_ptr != nullptr)
            {
                u32 const aligned_size = member.m_descr->m_typedescr->m_sizeof;
                for (s32 i = 0; i < array_size; ++i)
                {
                    JsonMember e;
                    e.m_descr    = member.m_descr;
                    e.m_data_ptr = array_ptr + (i * aligned_size);
                    doc.writeIndent();
                    JsonEncodeValue(e, doc, error_message);
                    doc.end(i == (array_size - 1));
                }
            }
            doc.endArray();
            return true;
        }

        static bool JsonEncodeObject(JsonObject& object, jsondoc_t& doc, char const*& error_message)
        {
            doc.startObject();
            JsonObjectTypeDef* objtype = object.m_descr->as_object_type();
            s32 const          n       = objtype->m_member_count;
            for (s32 i = 0; i < n; ++i)
            {
                JsonMember member(&objtype->m_members[i]);
                member.m_data_ptr = member.get_member_ptr(object);

                if (member.is_array() || member.is_array_ptr())
                {
                    doc.startField(member.m_descr->m_name);
                    if (!JsonEncodeArray(object, member, doc, error_message))
                    {
                        return false;
                    }
                    doc.end(i == (n - 1));
                }
                else
                {
                    // Do not emit this field if it is a pointer to a type and that pointer is null
                    if (!member.is_pointer() || member.get_pointer() != nullptr)
                    {
                        doc.startField(member.m_descr->m_name);
                        JsonEncodeValue(member, doc, error_message);
                        doc.end(i == (n - 1));
                    }
                }
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
            else if (member.is_enum())
            {
                doc.writeValueEnum(member);
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
            doc.m_json_text_end   = json_text_end - 1;

            doc.m_indent = 0;

            if (!JsonEncodeObject(root_object, doc, error_message))
            {
                return false;
            }
            return true;
        }

    } // namespace json

} // namespace xcore
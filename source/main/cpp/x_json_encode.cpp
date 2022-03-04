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
        struct jsondoc_t
        {
            char*       m_json_text;
            char const* m_json_text_end;
            s32 m_indent;

            void writeString(char const* str)
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
                    writeString(" ");
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

            void writeValueInt64(s64 field_value)
            {
                if (field_value < 0)
                {
                    writeString("-");
                    field_value = -field_value;
                }

                // convert 64 bit integer to string
                char buffer[32];
                s32 len = 0;
                do
                {
                    buffer[len++] = '0' + (field_value % 10);
                    field_value /= 10;
                } while (field_value > 0);

                // write string
                while (len > 0)
                {
                    --len;
                    writeString(&buffer[len]);
                }
            }

            void writeValueUInt64(u64 field_value)
            {
                // convert 64 bit integer to string
                char buffer[32];
                s32 len = 0;
                do
                {
                    buffer[len++] = '0' + (field_value % 10);
                    field_value /= 10;
                } while (field_value > 0);

                // write string
                while (len > 0)
                {
                    --len;
                    writeString(&buffer[len]);
                }
            }

            void writeValueFloat(f32 field_value)
            {
                s32 len = ftoa(field_value, m_json_text, m_json_text_end);
                m_json_text += len;
            }
        };

        bool JsonEncodeObject(JsonObject& object, s32 indentation_level, char*& json_text, char const* json_text_end, char const*& error_message) {}

        bool JsonEncode(JsonObject& root_object, char* json_text, char const* json_text_end, char const*& error_message) { return false; }

    } // namespace json

} // namespace xcore
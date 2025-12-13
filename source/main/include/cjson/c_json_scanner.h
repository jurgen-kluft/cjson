#ifndef __CJSON_JSON_SCANNER_H__
#define __CJSON_JSON_SCANNER_H__
#include "ccore/c_target.h"
#ifdef USE_PRAGMA_ONCE
#    pragma once
#endif

#include "ccore/c_debug.h"
#include "cjson/c_json_allocator.h"

namespace ncore
{
    namespace njson
    {
        struct JsonAllocator;

        namespace nscanner
        {
            struct JsonValue;

            struct JsonNumberValue
            {
                const char* m_String;
                const char* m_End;
            };

            struct JsonStringValue
            {
                const char* m_String;
                const char* m_End;
            };

            struct JsonLinkedValue
            {
                const JsonValue* m_Value;
                JsonLinkedValue* m_Next;
            };

            struct JsonArrayValue
            {
                i32              m_Count;
                JsonLinkedValue* m_LinkedList;
            };

            struct JsonNamedValue
            {
                const JsonStringValue* m_Name;
                const JsonValue*       m_Value;
            };

            struct JsonLinkedNamedValue
            {
                const JsonNamedValue* m_NamedValue;
                JsonLinkedNamedValue* m_Next;
            };

            struct JsonObjectValue
            {
                i32                   m_Count;
                JsonLinkedNamedValue* m_LinkedList;
            };

            struct JsonValue
            {
                enum Type
                {
                    kNull    = 1,
                    kObject  = 3,
                    kArray   = 4,
                    kString  = 5,
                    kNumber  = 6,
                    kBoolean = 7,
                };
                i32 m_Type;

                union value_t
                {
                    JsonNumberValue m_Number;
                    JsonStringValue m_String;
                    JsonArrayValue  m_Array;
                    JsonObjectValue m_Object;
                };
                value_t m_Value;

                inline bool IsArray() const { return m_Type == kArray; }
                inline bool IsObject() const { return m_Type == kObject; }
                inline bool IsString() const { return m_Type == kString; }
                inline bool IsNumber() const { return m_Type == kNumber; }
                inline bool IsBoolean() const { return m_Type == kBoolean; }
                inline bool IsNull() const { return m_Type == kNull; }

                const struct JsonObjectValue* AsObject() const;
                const struct JsonNumberValue* AsNumber() const;
                const struct JsonStringValue* AsString() const;
                const struct JsonArrayValue*  AsArray() const;

                const JsonLinkedValue*      ArrayHead(i32& count) const;
                const JsonLinkedNamedValue* MemberHead(i32& count) const;
            };

            inline const JsonObjectValue* JsonValue::AsObject() const
            {
                if (kObject == m_Type)
                    return &m_Value.m_Object;
                return nullptr;
            }

            inline const JsonArrayValue* JsonValue::AsArray() const
            {
                if (kArray == m_Type)
                    return &m_Value.m_Array;
                return nullptr;
            }

            inline const JsonStringValue* JsonValue::AsString() const
            {
                if (kString == m_Type)
                    return &m_Value.m_String;
                return nullptr;
            }

            inline const JsonNumberValue* JsonValue::AsNumber() const
            {
                if (kNumber == m_Type)
                    return &m_Value.m_Number;
                return nullptr;
            }

            inline const JsonLinkedValue* JsonValue::ArrayHead(i32& count) const
            {
                const JsonArrayValue* array = AsArray();
                ASSERT(array);
                count = array->m_Count;
                return array->m_LinkedList;
            }

            inline const JsonLinkedNamedValue* JsonValue::MemberHead(i32& count) const
            {
                const JsonObjectValue* object = AsObject();
                ASSERT(object);
                count = object->m_Count;
                return object->m_LinkedList;
            }

            // Scan JSON text into a lightweight JsonValue structure, there are no conversions happening, just determining [type, start-end] of objects, arrays, strings,
            // numbers, booleans and nulls.
            // All necessary JSON objects and their properties are allocated from 'allocator', you cannott free str/end, since all JSON objects reference the original text.
            const JsonValue* Scan(char const* str, char const* end, JsonAllocator* allocator, char const*& error_message);

        } // namespace nscanner
    } // namespace njson
} // namespace ncore

#endif // __CJSON_JSON_SCANNER_H__

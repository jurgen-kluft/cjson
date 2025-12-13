#ifndef __CJSON_JSON_H__
#define __CJSON_JSON_H__
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

        struct JsonValue;

        struct JsonBooleanValue
        {
            bool m_Boolean;
        };

        struct JsonNumberValue
        {
            u32 m_NumberType;
            union
            {
                f64 m_F64;
                u64 m_U64;
                s64 m_S64;
            };
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
            const char*      m_Name;
            const JsonValue* m_Value;
            JsonNamedValue*  m_Next;
        };

        struct JsonObjectValue
        {
            i32             m_Count;
            JsonNamedValue* m_LinkedList;
        };

        struct JsonValue
        {
            enum Type
            {
                kNull    = 1,
                kBoolean = 2,
                kObject  = 3,
                kArray   = 4,
                kString  = 5,
                kNumber  = 6,
            };
            i32 m_Type;

            union value_t
            {
                JsonBooleanValue m_Boolean;
                JsonNumberValue  m_Number;
                JsonStringValue  m_String;
                JsonArrayValue   m_Array;
                JsonObjectValue  m_Object;
            };
            value_t m_Value;

            inline bool IsArray() const { return m_Type == kArray; }
            inline bool IsObject() const { return m_Type == kObject; }
            inline bool IsString() const { return m_Type == kString; }
            inline bool IsNumber() const { return m_Type == kNumber; }
            inline bool IsBoolean() const { return m_Type == kBoolean; }
            inline bool IsNull() const { return m_Type == kNull; }

            const struct JsonObjectValue*  AsObject() const;
            const struct JsonNumberValue*  AsNumber() const;
            const struct JsonStringValue*  AsString() const;
            const struct JsonArrayValue*   AsArray() const;
            const struct JsonBooleanValue* AsBoolean() const;

            bool        GetBoolean() const;
            double      GetNumber() const;
            const char* GetString() const;
            bool        GetColor(u32& color) const;

            const JsonLinkedValue* Head(i32& count) const;
            const JsonValue*       Find(const char* key) const;
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

        inline const JsonBooleanValue* JsonValue::AsBoolean() const
        {
            if (kBoolean == m_Type)
                return &m_Value.m_Boolean;
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

        inline const JsonLinkedValue* JsonValue::Head(i32& count) const
        {
            const JsonArrayValue* array = AsArray();
            ASSERT(array);
            count = array->m_Count;
            return array->m_LinkedList;
        }

        inline double JsonValue::GetNumber() const
        {
            const JsonNumberValue* num = AsNumber();
            ASSERT(num);
            return num->m_F64;
        }

        inline const char* JsonValue::GetString() const
        {
            const JsonStringValue* str = AsString();
            ASSERT(str);
            return str->m_String;
        }

        inline bool JsonValue::GetBoolean() const
        {
            const JsonBooleanValue* b = AsBoolean();
            ASSERT(b);
            return b->m_Boolean;
        }

        // Parse JSON text into a JsonValue document, when an error occurs the return value is nullptr and the error description is set in error_message
        // which is allocated from 'scratch'.
        // All necessary JSON values and their properties are allocated from 'allocator', you can thus free str/end as well as 'scratch' after calling
        // this function.
        const JsonValue* Parse(char const* str, char const* end, JsonAllocator* allocator, JsonAllocator* scratch, char const*& error_message);

    } // namespace njson
} // namespace ncore

#endif // __CJSON_JSON_H__

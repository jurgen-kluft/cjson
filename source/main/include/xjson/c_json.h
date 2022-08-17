#ifndef __XBASE_JSON_H__
#define __XBASE_JSON_H__
#include "xbase/x_target.h"
#ifdef USE_PRAGMA_ONCE
#pragma once
#endif

#include "xbase/x_debug.h"

namespace xcore
{
    namespace json
    {
        struct JsonAllocator;

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
            u16 m_Type;

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

            double      GetNumber() const;
            const char* GetString() const;
            bool        GetColor(u32& color) const;
            bool        GetBoolean() const;

            const JsonValue* Elem(int index) const;
            const JsonValue* Find(const char* key) const;
        };

        struct JsonBooleanValue : JsonValue
        {
            u16 m_Boolean;
        };

        struct JsonNumberValue : JsonValue
        {
            double m_Number;
        };

        struct JsonStringValue : JsonValue
        {
            const char* m_String;
        };

        struct JsonArrayValue : JsonValue
        {
            int               m_Count;
            const JsonValue** m_Values;
        };

        struct JsonObjectValue : JsonValue
        {
            int               m_Count;
            const char**      m_Names;
            const JsonValue** m_Values;
        };

        inline const JsonObjectValue* JsonValue::AsObject() const
        {
            if (kObject == m_Type)
                return static_cast<const JsonObjectValue*>(this);
            return nullptr;
        }

        inline const JsonArrayValue* JsonValue::AsArray() const
        {
            if (kArray == m_Type)
                return static_cast<const JsonArrayValue*>(this);
            return nullptr;
        }

        inline const JsonBooleanValue* JsonValue::AsBoolean() const
        {
            if (kBoolean == m_Type)
                return static_cast<const JsonBooleanValue*>(this);
            return nullptr;
        }

        inline const JsonStringValue* JsonValue::AsString() const
        {
            if (kString == m_Type)
                return static_cast<const JsonStringValue*>(this);
            return nullptr;
        }

        inline const JsonNumberValue* JsonValue::AsNumber() const
        {
            if (kNumber == m_Type)
                return static_cast<const JsonNumberValue*>(this);
            return nullptr;
        }

        inline const JsonValue* JsonValue::Elem(int index) const
        {
            const JsonArrayValue* array = AsArray();
            ASSERT(index < array->m_Count);
            return array->m_Values[index];
        }

        inline double JsonValue::GetNumber() const
        {
            const JsonNumberValue* num = AsNumber();
            ASSERT(num);
            return num->m_Number;
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
        const JsonValue* JsonParse(char const* str, char const* end, JsonAllocator* allocator, JsonAllocator* scratch, char const*& error_message);

    } // namespace json
} // namespace xcore

#endif // __XBASE_JSON_H__
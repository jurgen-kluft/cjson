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
        struct MemAllocLinear;

        MemAllocLinear* CreateMemAllocLinear(u32 size);
        void            DestroyMemAllocLinear(MemAllocLinear* alloc);

        struct JsonValue
        {
            enum Type
            {
                kNull,
                kBoolean,
                kObject,
                kArray,
                kString,
                kNumber
            };

            Type m_Type;

            const struct JsonObjectValue*  AsObject() const;
            const struct JsonNumberValue*  AsNumber() const;
            const struct JsonStringValue*  AsString() const;
            const struct JsonArrayValue*   AsArray() const;
            const struct JsonBooleanValue* AsBoolean() const;

            double      GetNumber() const;
            const char* GetString() const;
            bool        GetBoolean() const;

            const JsonValue* Elem(int index) const;
            const JsonValue* Find(const char* key) const;
        };

        struct JsonBooleanValue : JsonValue
        {
            bool m_Boolean;
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

        inline const JsonValue* JsonValue::Find(const char* key) const
        {
            const JsonObjectValue* obj = AsObject();
            for (int i = 0, count = obj->m_Count; i < count; ++i)
            {
                const char* cmp_key = key;
                const char* obj_key = obj->m_Names[i];
                while (*cmp_key != 0 && *obj_key != 0 && *cmp_key == *obj_key)
                    ++cmp_key, ++obj_key;
                if (*cmp_key == 0 && *obj_key == 0)
                    return obj->m_Values[i];
            }
            return nullptr;
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
        const JsonValue* JsonParse(char* str, char const* end, MemAllocLinear* allocator, MemAllocLinear* scratch, char const*& error_message);

    } // namespace json
} // namespace xcore

#endif // __XBASE_JSON_H__
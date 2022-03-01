#include "xbase/x_allocator.h"
#include "xbase/x_context.h"
#include "xbase/x_printf.h"
#include "xbase/x_runes.h"
#include "xjson/x_json.h"
#include "xjson/x_json_utils.h"
#include "xjson/x_json_allocator.h"
#include "xjson/x_json_decode.h"

namespace xcore
{
    namespace json
    {
        static void CAllocBool(JsonAllocator* alloc, s32 n, void*& ptr) { ptr = alloc->AllocateArray<bool>(n); }

        static JsonMemberType sJsonTypeBool = {
            JsonMemberType::TypeBool,
            JsonMemberType::TypeNull,
            CAllocBool,
            nullptr,
        };
        static JsonMemberType sJsonTypeBoolPtr = {
            JsonMemberType::TypeBool | JsonMemberType::TypePtr,
            JsonMemberType::TypeNull,
            CAllocBool,
            nullptr,
        };
        static JsonMemberType sJsonTypeBoolVector = {
            JsonMemberType::TypeBool | JsonMemberType::TypeVector,
            JsonMemberType::TypeNull,
            CAllocBool,
            nullptr,
        };
        static JsonMemberType sJsonTypeBoolCArray = {
            JsonMemberType::TypeBool | JsonMemberType::TypeCarray,
            JsonMemberType::TypeNull,
            CAllocBool,
            nullptr,
        };

        JsonMemberType const* JsonTypeBool = &sJsonTypeBool;
        JsonMemberType const* JsonTypeBoolPtr = &sJsonTypeBoolPtr;
        JsonMemberType const* JsonTypeBoolVector = &sJsonTypeBoolVector;
        JsonMemberType const* JsonTypeBoolCArray = &sJsonTypeBoolCArray;

    } // namespace json
} // namespace xcore
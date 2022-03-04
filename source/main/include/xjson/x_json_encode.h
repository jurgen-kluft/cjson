#ifndef __XBASE_JSON_ENCODE_H__
#define __XBASE_JSON_ENCODE_H__
#include "xbase/x_target.h"
#ifdef USE_PRAGMA_ONCE
#pragma once
#endif

namespace xcore
{
    namespace json
    {
        struct JsonObject;
        bool JsonEncode(JsonObject& json_root, char* json_text, char const* json_text_end, char const*& error_message);
    } // namespace json
} // namespace xcore

#endif // __XBASE_JSON_H__
#ifndef __CJSON_JSON_ENCODE_H__
#define __CJSON_JSON_ENCODE_H__
#include "cbase/c_target.h"
#ifdef USE_PRAGMA_ONCE
#pragma once
#endif

namespace ncore
{
    namespace json
    {
        struct JsonObject;
        bool JsonEncode(JsonObject& json_root, char* json_text, char const* json_text_end, char const*& error_message);
    } // namespace json
} // namespace ncore

#endif // __CJSON_JSON_ENCODE_H__
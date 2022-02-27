#include "xbase/x_target.h"
#include "xbase/x_runes.h"
#include "xjson/x_json.h"

#include "xunittest/xunittest.h"

using namespace xcore;

extern unsigned char   kyria_json[];
extern unsigned int    kyria_json_len;

UNITTEST_SUITE_BEGIN(xjson)
{
    UNITTEST_FIXTURE(parse)
    {
        UNITTEST_FIXTURE_SETUP() {}
        UNITTEST_FIXTURE_TEARDOWN() {}

        UNITTEST_TEST(test)
        {
            const char* json = (const char*)kyria_json;
            const char* json_end = json + kyria_json_len;
            json::MemAllocLinear* lma = json::CreateMemAllocLinear(16384);
            json::MemAllocLinear* lsa = json::CreateMemAllocLinear(8192);
            
            const char* errmsg;
            json::JsonValue const* root = json::JsonParse(json, json_end, lma, lsa, errmsg);
            CHECK_NULL(errmsg);
            CHECK_TRUE(root->m_Type == json::JsonValue::kObject);

            json::DestroyMemAllocLinear(lsa);
            json::DestroyMemAllocLinear(lma);
        }
    }
}
UNITTEST_SUITE_END
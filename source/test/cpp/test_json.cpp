#include "cbase/x_target.h"
#include "cbase/x_runes.h"
#include "xjson/x_json.h"
#include "xjson/x_json_allocator.h"

#include "cunittest/cunittest.h"

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
            json::JsonAllocator* lma = json::CreateAllocator(16384);
            json::JsonAllocator* lsa = json::CreateAllocator(8192);
            
            const char* errmsg;
            json::JsonValue const* root = json::JsonParse(json, json_end, lma, lsa, errmsg);
            CHECK_NULL(errmsg);
            CHECK_TRUE(root->m_Type == json::JsonValue::kObject);

            json::DestroyAllocator(lsa);
            json::DestroyAllocator(lma);
        }
    }
}
UNITTEST_SUITE_END
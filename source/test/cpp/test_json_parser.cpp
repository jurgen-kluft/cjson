#include "ccore/c_target.h"
#include "cbase/c_runes.h"
#include "cjson/c_json_parser.h"
#include "cjson/c_json_allocator.h"

#include "cunittest/cunittest.h"

using namespace ncore;

extern unsigned char kyria_json[];
extern unsigned int  kyria_json_len;

UNITTEST_SUITE_BEGIN(json_parser)
{
    UNITTEST_FIXTURE(parse)
    {
        UNITTEST_FIXTURE_SETUP() {}
        UNITTEST_FIXTURE_TEARDOWN() {}

        UNITTEST_ALLOCATOR;

        UNITTEST_TEST(test)
        {
            const char* json     = (const char*)kyria_json;
            const char* json_end = json + kyria_json_len;

            njson::JsonAllocator lma;
            njson::JsonAllocator lsa;
            lma.Init(Allocator, 16384, "json_main");
            lsa.Init(Allocator, 8192, "json_scratch");

            const char*             errmsg;
            njson::JsonValue const* root = njson::Parse(json, json_end, &lma, &lsa, errmsg);
            CHECK_NULL(errmsg);
            CHECK_TRUE(root->m_Type == njson::JsonValue::kObject);

            lsa.Destroy();
            lma.Destroy();
        }
    }
}
UNITTEST_SUITE_END

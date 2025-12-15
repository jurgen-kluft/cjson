#include "ccore/c_target.h"
#include "cbase/c_runes.h"
#include "cjson/c_json_scanner.h"
#include "cjson/c_json_allocator.h"

#include "cunittest/cunittest.h"

using namespace ncore;

extern unsigned char data_kyria[];
extern unsigned int  data_kyria_len;

UNITTEST_SUITE_BEGIN(json_scanner)
{
    UNITTEST_FIXTURE(scan)
    {
        UNITTEST_FIXTURE_SETUP() {}
        UNITTEST_FIXTURE_TEARDOWN() {}

        UNITTEST_ALLOCATOR;

        UNITTEST_TEST(test)
        {
            const char* json     = (const char*)data_kyria;
            const char* json_end = json + data_kyria_len;

            njson::JsonAllocator lma;
            lma.Init(Allocator, 16384, "json_main");

            const char*             errmsg;
            njson::nscanner::JsonValue const* root = njson::nscanner::Scan(json, json_end, &lma, errmsg);
            CHECK_NULL(errmsg);
            CHECK_TRUE(root->m_Type == njson::nscanner::JsonValue::kObject);

            lma.Destroy();
        }
    }
}
UNITTEST_SUITE_END

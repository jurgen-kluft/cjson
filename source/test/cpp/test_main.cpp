#include "cbase/x_base.h"
#include "cbase/x_allocator.h"
#include "cbase/x_console.h"
#include "cbase/x_context.h"

#include "cunittest/cunittest.h"
#include "cunittest/private/ut_ReportAssert.h"

UNITTEST_SUITE_LIST(xJsmnUnitTest);
UNITTEST_SUITE_DECLARE(xJsmnUnitTest, xjson);
UNITTEST_SUITE_DECLARE(xJsmnUnitTest, xjson_decode);

namespace xcore
{
	// Our own assert handler
	class UnitTestAssertHandler : public xcore::asserthandler_t
	{
	public:
		UnitTestAssertHandler()
		{
			NumberOfAsserts = 0;
		}

		virtual bool handle_assert(u32 &flags, const char *fileName, s32 lineNumber, const char *exprString, const char *messageString)
		{
			UnitTest::reportAssert(exprString, fileName, lineNumber);
			NumberOfAsserts++;
			return false;
		}

		xcore::s32 NumberOfAsserts;
	};

	class UnitTestAllocator : public UnitTest::Allocator
	{
		xcore::alloc_t *mAllocator;

	public:
		UnitTestAllocator(xcore::alloc_t *allocator) { mAllocator = allocator; }
		virtual void *Allocate(size_t size) { return mAllocator->allocate((u32)size, sizeof(void *)); }
		virtual size_t Deallocate(void *ptr) { return mAllocator->deallocate(ptr); }
	};

	class TestAllocator : public alloc_t
	{
		alloc_t *mAllocator;

	public:
		TestAllocator(alloc_t *allocator) : mAllocator(allocator) {}

		virtual void *v_allocate(u32 size, u32 alignment)
		{
			UnitTest::IncNumAllocations();
			return mAllocator->allocate(size, alignment);
		}

		virtual u32 v_deallocate(void *mem)
		{
			UnitTest::DecNumAllocations();
			return mAllocator->deallocate(mem);
		}

		virtual void v_release()
		{
			mAllocator = NULL;
		}
	};
} // namespace xcore

xcore::alloc_t *gTestAllocator = NULL;
xcore::UnitTestAssertHandler gAssertHandler;

bool gRunUnitTest(UnitTest::TestReporter &reporter)
{
	cbase::init();

#ifdef TARGET_DEBUG
	xcore::context_t::set_assert_handler(&gAssertHandler);
#endif
	xcore::console->write("Configuration: ");
	xcore::console->writeLine(TARGET_FULL_DESCR_STR);

	xcore::alloc_t* systemAllocator = xcore::context_t::system_alloc();
	xcore::UnitTestAllocator unittestAllocator( systemAllocator );
	UnitTest::SetAllocator(&unittestAllocator);

	xcore::console->write("Configuration: ");
	xcore::console->setColor(xcore::console_t::YELLOW);
	xcore::console->writeLine(TARGET_FULL_DESCR_STR);
	xcore::console->setColor(xcore::console_t::NORMAL);

	xcore::TestAllocator testAllocator(systemAllocator);
	gTestAllocator = &testAllocator;

	int r = UNITTEST_SUITE_RUN(reporter, xJsmnUnitTest);
	if (UnitTest::GetNumAllocations() != 0)
	{
		reporter.reportFailure(__FILE__, __LINE__, "cunittest", "memory leaks detected!");
		r = -1;
	}

	gTestAllocator->release();

	UnitTest::SetAllocator(NULL);
	xcore::context_t::set_system_alloc(systemAllocator);

	cbase::exit();
	return r == 0;
}

#include "xbase/x_base.h"
#include "xbase/x_allocator.h"
#include "xbase/x_console.h"
#include "xbase/x_context.h"

#include "xfile/x_file.h"

#include "xunittest/xunittest.h"
#include "xunittest/private/ut_ReportAssert.h"

UNITTEST_SUITE_LIST(xFileIOUnitTest);

UNITTEST_SUITE_DECLARE(xFileIOUnitTest, test_file);
UNITTEST_SUITE_DECLARE(xFileIOUnitTest, test_file_stream);

namespace xcore
{
	class UnitTestAssertHandler : public xcore::asserthandler_t
	{
	public:
		UnitTestAssertHandler()
		{
			NumberOfAsserts = 0;
		}

		virtual bool	handle_assert(u32& flags, const char* fileName, s32 lineNumber, const char* exprString, const char* messageString)
		{
			UnitTest::reportAssert(exprString, fileName, lineNumber);
			NumberOfAsserts++;
			return false;
		}

		xcore::s32		NumberOfAsserts;
	};

	class UnitTestAllocator : public UnitTest::Allocator
	{
		alloc_t*			mAllocator;
	public:
						UnitTestAllocator(alloc_t* allocator)				{ mAllocator = allocator; }
		virtual void*	Allocate(size_t size)								{ return mAllocator->allocate((u32)size, 4); }
		virtual size_t	Deallocate(void* ptr)								{ return mAllocator->deallocate(ptr); }
	};

	class TestAllocator : public alloc_t
	{
		alloc_t*				mAllocator;
	public:
							TestAllocator(alloc_t* allocator) : mAllocator(allocator) { }

		virtual const char*	name() const										{ return "xcore unittest test heap allocator"; }

		virtual void*		v_allocate(u32 size, u32 alignment)
		{
			UnitTest::IncNumAllocations();
			return mAllocator->allocate(size, alignment);
		}

		virtual u32 		v_deallocate(void* mem)
		{
			UnitTest::DecNumAllocations();
			return mAllocator->deallocate(mem);
		}

		virtual void		v_release()
		{
			mAllocator = NULL;
		}
	};
}

xcore::alloc_t *gTestAllocator = NULL;
xcore::UnitTestAssertHandler gAssertHandler;

bool gRunUnitTest(UnitTest::TestReporter& reporter)
{
	xbase::init();

#ifdef TARGET_DEBUG
	xcore::context_t::set_assert_handler(&gAssertHandler);
#endif
	xcore::console->write("Configuration: ");
	xcore::console->setColor(xcore::console_t::YELLOW);
	xcore::console->writeLine(TARGET_FULL_DESCR_STR);
	xcore::console->setColor(xcore::console_t::NORMAL);

	xcore::alloc_t* systemAllocator = xcore::context_t::system_alloc();
	xcore::UnitTestAllocator unittestAllocator( systemAllocator );
	UnitTest::SetAllocator(&unittestAllocator);

	xcore::TestAllocator testAllocator(systemAllocator);
	gTestAllocator = &testAllocator;
	xcore::context_t::set_system_alloc(&testAllocator);

	int r = UNITTEST_SUITE_RUN(reporter, xFileIOUnitTest);
	if (UnitTest::GetNumAllocations()!=0)
	{
		reporter.reportFailure(__FILE__, __LINE__, "xunittest", "memory leaks detected!");
		r = -1;
	}

	gTestAllocator->release();

	UnitTest::SetAllocator(NULL);
	xcore::context_t::set_system_alloc(systemAllocator);

	xbase::exit();
	return r==0;
}


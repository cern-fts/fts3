#include "Executor.h"

#ifdef FTS3_COMPILE_WITH_UNITTEST_NEW

using namespace fts3::common;

// define it for unit tests purposes
bool stopThreads = false;

BOOST_AUTO_TEST_SUITE( common )
BOOST_AUTO_TEST_SUITE(ExecutorTest)

struct ExecTask
{
	virtual int execute()
	{
		return 1;
	}
};

struct ExecFailed : public ExecTask
{
	int execute()
	{
		return 0;
	}
};

BOOST_AUTO_TEST_CASE (Executor_test)
{
	// create executor
	Executor<ExecTask> exec;
	// add 3 successful tasks
	exec.put(new ExecTask);
	exec.put(new ExecTask);
	exec.put(new ExecTask);
	// add 2 failed tasks
	exec.put(new ExecFailed);
	exec.put(new ExecFailed);
	// we are not going to put more tasks
	exec.noMoreData();

	// it should be active
	BOOST_CHECK(exec.isActive());

	// execute all tasks
	exec.execute();

	BOOST_CHECK_EQUAL(exec.executed(), 3);
}

BOOST_AUTO_TEST_SUITE_END()
BOOST_AUTO_TEST_SUITE_END()
#endif // FTS3_COMPILE_WITH_UNITTESTS

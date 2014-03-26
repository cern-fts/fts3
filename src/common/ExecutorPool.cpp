#include "ExecutorPool.h"

#ifdef FTS3_COMPILE_WITH_UNITTEST_NEW

using namespace fts3::common;

// define it for unit tests purposes
//bool stopThreads = false;

BOOST_AUTO_TEST_SUITE( common )
BOOST_AUTO_TEST_SUITE(ExecutorPoolTest)

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

BOOST_AUTO_TEST_CASE (ExecutorPool_test)
{
    ExecutorPool<ExecTask> pool(1);
    pool.add(new ExecTask);
    pool.add(new ExecTask);
    pool.add(new ExecTask);
    pool.add(new ExecFailed);
    pool.add(new ExecFailed);

    pool.join();

    BOOST_CHECK_EQUAL(pool.executed(), 3);
}

BOOST_AUTO_TEST_SUITE_END()
BOOST_AUTO_TEST_SUITE_END()
#endif // FTS3_COMPILE_WITH_UNITTESTS

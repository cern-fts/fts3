/*
 * ThreadPool.cpp
 *
 *  Created on: 23 May 2014
 *      Author: simonm
 */

#include "ThreadPool.h"

#include <iostream>

#ifdef FTS3_COMPILE_WITH_UNITTEST_NEW

#include "unittest/testsuite.h"

BOOST_AUTO_TEST_SUITE( common )
BOOST_AUTO_TEST_SUITE(ThreadPoolTest)

struct EmptyTask
{
    void run(boost::any const &) {}
};

BOOST_AUTO_TEST_CASE (ThreadPool_size)
{
    using namespace fts3::common;

    ThreadPool<EmptyTask> tp(10);
    BOOST_CHECK_EQUAL(tp.size(), 10);
    tp.join();
}

struct IdTask
{
    IdTask(boost::thread::id & id) : id(id) {}

    void run(boost::any const &)
    {
//		boost::this_thread::sleep_for(boost::chrono::seconds(1));
        sleep(1);
        id = boost::this_thread::get_id();
    }

    boost::thread::id & id;
};

BOOST_AUTO_TEST_CASE (ThreadPool_start)
{
    using namespace fts3::common;

    boost::thread::id ids[3];

    ThreadPool<IdTask> one_thread(1);
    one_thread.start(new IdTask(ids[0]));
    one_thread.start(new IdTask(ids[1]));
    one_thread.join();

    BOOST_CHECK_EQUAL(ids[0], ids[1]);

    ThreadPool<IdTask> two_threads(2);
    two_threads.start(new IdTask(ids[0]));
    two_threads.start(new IdTask(ids[1]));
    two_threads.join();

    BOOST_CHECK_NE(ids[0], ids[1]);

    ThreadPool<IdTask> three_tasks(2);
    three_tasks.start(new IdTask(ids[0]));
    three_tasks.start(new IdTask(ids[1]));
    three_tasks.start(new IdTask(ids[2]));
    three_tasks.join();

    BOOST_CHECK_NE(ids[0], ids[1]);
    BOOST_CHECK(ids[2] == ids[0] || ids[2] == ids[1]);
}

struct SleepyTask
{
    SleepyTask(bool & done) : done(done) {}

    void run(boost::any const &)
    {
//		boost::this_thread::sleep_for(boost::chrono::seconds(1));
        sleep(1);
        done = true;
    }

    bool & done;
};

BOOST_AUTO_TEST_CASE (ThreadPool_join)
{
    using namespace fts3::common;

    bool done = false;

    ThreadPool<SleepyTask> tp(1);
    tp.start(new SleepyTask(done));
    tp.join();
    BOOST_CHECK(done);
}

struct InfiniteTask
{
    void run(boost::any const &)
    {
        while(true) boost::this_thread::interruption_point();
    }
};

BOOST_AUTO_TEST_CASE (ThreadPool_interrupt)
{
    using namespace fts3::common;

    ThreadPool<InfiniteTask> tp(1);
    tp.start(new InfiniteTask());
    tp.interrupt();
    tp.join();
}

struct InitTask
{
	InitTask(std::string & str) : str(str) {}

	void run(boost::any const & data)
	{
		if (data.empty()) return;

		std::string d = boost::any_cast<std::string>(data);

		str += d;
	}

	std::string & str;
};

void init_func(boost::any & data)
{
	data = std::string(".00$");
}

BOOST_AUTO_TEST_CASE (ThreadPool_init_func)
{
	using namespace fts3::common;

	std::string ret[2] = {"10", "100"};

	ThreadPool<InitTask> tp (2, init_func);

	tp.start(new InitTask(ret[0]));
	tp.start(new InitTask(ret[1]));
	tp.join();

	BOOST_CHECK_EQUAL(ret[0], "10.00$");
	BOOST_CHECK_EQUAL(ret[1], "100.00$");
}

struct init_obj
{
	void operator() (boost::any & data)
	{
		data = std::string(".00$");
	}
};

BOOST_AUTO_TEST_CASE (ThreadPool_init_obj)
{
	using namespace fts3::common;

	std::string ret[2] = {"10", "100"};

	init_obj obj;

	ThreadPool<InitTask, init_obj> tp (2, obj);

	tp.start(new InitTask(ret[0]));
	tp.start(new InitTask(ret[1]));
	tp.join();

	BOOST_CHECK_EQUAL(ret[0], "10.00$");
	BOOST_CHECK_EQUAL(ret[1], "100.00$");
}

BOOST_AUTO_TEST_SUITE_END()
BOOST_AUTO_TEST_SUITE_END()

#endif // FTS3_COMPILE_WITH_UNITTESTS


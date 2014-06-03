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

BOOST_AUTO_TEST_CASE (ThreadPool_size)
{
	using namespace fts3::common;

	struct EmptyTask { void run() {} };

	ThreadPool<EmptyTask> tp(10);
	BOOST_CHECK_EQUAL(tp.size(), 10);
	tp.join();
}

BOOST_AUTO_TEST_CASE (ThreadPool_start)
{
	using namespace fts3::common;

	boost::thread::id ids[3];

	struct IdTask
	{
		IdTask(boost::thread::id & id) : id(id) {}

		void run()
		{
			boost::this_thread::sleep_for(boost::chrono::seconds(1));
			id = boost::this_thread::get_id();
		}

		boost::thread::id & id;
	};

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

BOOST_AUTO_TEST_CASE (ThreadPool_join)
{
	using namespace fts3::common;

	bool done = false;

	struct SleepyTask
	{
		SleepyTask(bool & done) : done(done) {}

		void run()
		{
			boost::this_thread::sleep_for(boost::chrono::seconds(1));
			done = true;
		}

		bool & done;
	};

	ThreadPool<SleepyTask> tp(1);
	tp.start(new SleepyTask(done));
	tp.join();
	BOOST_CHECK(done);
}

BOOST_AUTO_TEST_CASE (ThreadPool_interrupt)
{
	using namespace fts3::common;

	struct InfiniteTask
	{
		void run()
		{
			while(true) boost::this_thread::interruption_point();
		}
	};

	ThreadPool<InfiniteTask> tp(1);
	tp.start(new InfiniteTask());
	tp.interrupt();
	tp.join();
}

BOOST_AUTO_TEST_SUITE_END()
BOOST_AUTO_TEST_SUITE_END()

#endif // FTS3_COMPILE_WITH_UNITTESTS


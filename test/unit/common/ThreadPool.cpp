/*
 * Copyright (c) CERN 2013-2015
 *
 * Copyright (c) Members of the EMI Collaboration. 2010-2013
 *  See  http://www.eu-emi.eu/partners for details on the copyright
 *  holders.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <boost/test/included/unit_test.hpp>

#include <boost/any.hpp>

#include "common/ThreadPool.h"

using fts3::common::ThreadPool;


BOOST_AUTO_TEST_SUITE(common)
BOOST_AUTO_TEST_SUITE(ThreadPoolTest)

struct EmptyTask
{
    void run(boost::any const &) {}
};


BOOST_AUTO_TEST_CASE (ThreadPoolSize)
{
    ThreadPool<EmptyTask> tp(10);
    BOOST_CHECK_EQUAL(tp.size(), 10);
    tp.join();
}


struct IdTask
{
    IdTask(boost::thread::id & id) : id(id) {}

    void run(boost::any const &)
    {
        boost::this_thread::sleep(boost::posix_time::seconds(1));
        id = boost::this_thread::get_id();
    }

    boost::thread::id & id;
};


BOOST_AUTO_TEST_CASE (ThreadPoolStart)
{
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
        boost::this_thread::sleep(boost::posix_time::seconds(1));
        done = true;
    }

    bool & done;
};


BOOST_AUTO_TEST_CASE (ThreadPoolJoin)
{
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
        while(true) {
            boost::this_thread::interruption_point();
        }
    }
};


BOOST_AUTO_TEST_CASE (ThreadPoolInterrupt)
{
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


void init_func(boost::any &data)
{
    data = std::string(".00$");
}


BOOST_AUTO_TEST_CASE (ThreadPoolInitFunc)
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


struct InitCallableObject
{
    void operator() (boost::any & data)
    {
        data = std::string(".00$");
    }
};


BOOST_AUTO_TEST_CASE (ThreadPoolInitObj)
{
    using namespace fts3::common;

    std::string ret[2] = {"10", "100"};

    InitCallableObject obj;

    ThreadPool<InitTask, InitCallableObject> tp (2, obj);

    tp.start(new InitTask(ret[0]));
    tp.start(new InitTask(ret[1]));
    tp.join();

    BOOST_CHECK_EQUAL(ret[0], "10.00$");
    BOOST_CHECK_EQUAL(ret[1], "100.00$");
}


void zero_func(boost::any & data)
{
    data = 0;
}


struct IncTask
{
    void run(boost::any & data)
    {
        if (data.empty()) return;

        int & i = boost::any_cast<int &>(data);
        ++i;
    }
};


BOOST_AUTO_TEST_CASE (ThreadPoolReduce)
{
    using namespace fts3::common;

    ThreadPool<IncTask> tp (2, zero_func);
    tp.start(new IncTask);
    tp.start(new IncTask);
    tp.start(new IncTask);
    tp.start(new IncTask);
    tp.join();

    int result = tp.reduce(std::plus<int>());
    BOOST_CHECK_EQUAL(result, 4);

    result = tp.reduce(std::minus<int>());
    BOOST_CHECK_EQUAL(result, -4);

    ThreadPool<IncTask> tp2 (2, zero_func);
    tp2.start(new IncTask);
    tp2.join();
    result = tp2.reduce(std::plus<int>());
    BOOST_CHECK_EQUAL(result, 1);
}


BOOST_AUTO_TEST_SUITE_END()
BOOST_AUTO_TEST_SUITE_END()

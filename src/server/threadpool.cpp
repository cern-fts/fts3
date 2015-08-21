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

#include "threadpool.h"

#ifdef FTS3_COMPILE_WITH_UNITTEST
#include "unittest/testsuite.h"
#endif // FTS3_COMPILE_WITH_UNITTESTS

/* ---------------------------------------------------------------------- */

FTS3_SERVER_NAMESPACE_START

namespace ThreadPool
{

ThreadPool::ThreadPool(const size_t queueSize, const size_t workerNum)
    : _queue(queueSize)
{
    int c = 1;
    for (size_t i = 0; i < workerNum; ++i) _workers.push_back(new Worker(_thgrp, c++));

    FTS3_COMMON_LOGGER_NEWLOG(DEBUG) << "ThreadPool number of worker threads: " << workerNum << commit;
}

ThreadPool::~ThreadPool()
{
    // EMPTY
}

void ThreadPool::wait()
{
    _thgrp.join_all();
}

void ThreadPool::stop()
{
    FTS3_COMMON_MONITOR_START_CRITICAL
    _thgrp.interrupt_all();

    boost::ptr_vector<Worker>::iterator iter = _workers.begin();

    while (iter != _workers.end())
        {
            if (!boost::is_null(iter))
                {
                    //iter->cancel();
                    iter = _workers.erase(iter);
                }
            else
                {
                    ++iter;
                }
        }
    FTS3_COMMON_MONITOR_END_CRITICAL
}

ThreadPool::element_type ThreadPool::pop(const Timeout& td)
{
    element_type t(_queue.pop(td));
    return t;
}

#ifdef FTS3_COMPILE_WITH_UNITTEST

/** The operation stores a value in an external (referenced) value */
class TestOp
{
public:
    typedef int value_type;
    enum enabled_value_type {INVALID_VALUE, VALUE_1, VALUE_2, VALUE_3};

    TestOp
    (
        value_type& ret, /**< The external variable */
        int expected /**< The value to be stored */
    )
        : _ret(ret),
          _exp(expected)
    {};

    void operator () ()
    {
        _ret = _exp;
    };

    static value_type val(enabled_value_type e)
    {
        return static_cast<value_type>(e);
    }

private:
    value_type& _ret;
    const value_type _exp;
};
#if 0
bOOST_AUTO_TEST_CASE(ThreadPoolTest)
{
    FTS3_CONFIG_NAMESPACE::theServerConfig().read(0, NULL);
    // TODO
    for (int i = 0; i < 10; ++i)
        {
            TestOp::value_type val(TestOp::INVALID_VALUE);
            TestOp op(val, TestOp::VALUE_1);
            ThreadPool::ThreadPool::instance().enqueue(op);
        }

    ::sleep(1);

    /* Check if the initial value is correct */
    //CPPUNIT_ASSERT_EQUAL(TestOp::val(TestOp::INVALID_VALUE), val);
    /* Test if after invoking the task operation, the value changed */
    //CPPUNIT_ASSERT_EQUAL(TestOp::val(TestOp::VALUE_1), val);
}
#endif
#endif // FTS3_COMPILE_WITH_UNITTESTS

} // namespace ThreadPool

FTS3_SERVER_NAMESPACE_END

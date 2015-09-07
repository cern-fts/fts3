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

#include "thread_function.h"
#include "synchronizedqueue.h"
#include "../common/Timeout.h"

#ifdef FTS3_COMPILE_WITH_UNITTEST
#include "unittest/testsuite.h"
#endif // FTS3_COMPILE_WITH_UNITTESTS

using namespace fts3::server;

#ifdef FTS3_COMPILE_WITH_UNITTEST

struct SynchronizedQueueTest
{
    /** Create a context for every single test. A test involves:
     *   - a queue
     *   - the thread group: a test exits only if all the threads have been
     *     finished and joined
     *
     * When creating a test context, it sets up a new queue and a new thread group.
     * When the test context goes out of scope, its destructor waits while all the
     * threads have been joined. Mind, that in case of test failure, cppunit
     * will generate an assertion causing test context destructor call.
     *
     * The queue that is being tested contains integer elements. The pop and push
     * functions of the context are used to push and pop values to/from the internal
     * queue. Those operations are executed in separate threads, so the functions
     * basically create function objects that, upon invocation, pushes/pops the
     * values in the queue. The function object then passed to a new thread, then
     * we wait for a condition meaning that the operation successfully executed.
     * There is a timeout, so we can determine if the operation has been executed
     * properly or the thread is blocked on an operation (the queue is full, empty,
     * etc.)
     *
     * Remark: we must push only non-negative integers as the negative ones have
     * internal meanings.
     *
     * If a test failed due to a thread got blocked infinitely, the destructor
     * (and the whole test execution) will block. Should be optimized.
     */
    class _TestContext
    {
    public:
        typedef int queue_e_t;
        typedef SynchronizedQueue<queue_e_t, std::shared_ptr> queue_t;
        typedef std::shared_ptr<queue_t> queue_p_t;

        _TestContext() : _q(new queue_t(_QUEUE_SIZE)) {};

        ~_TestContext()
        {
            _thgrp.join_all();
        }

        /** Pop and return a value from the queue. There is a special value
         * representing timeout. The return value must be checked with isTimeout
         * to ensure that it represents a valid value */
        queue_e_t pop()
        {
            _PopFunc pf(_q);
            boost::function0<void> f = pf;
            _thgrp.create_thread(f);
            // The wait returns false if returned tue to timeout
            _WAIT_TIME().actualize();
            return pf.wait(_WAIT_TIME().getXtime()) ? pf.getVal() : _TIMEOUT_VALUE();
        }

        /** Push a value to the queue.
         *  return: true: value is pushed, false: timeout occured */
        bool push(const queue_e_t& v)
        {
            assert(v >= 0);
            _PushFunc pf(_q, v);
            boost::function0<void> f = pf;
            _thgrp.create_thread(f);
            _WAIT_TIME().actualize();
            return pf.wait(_WAIT_TIME().getXtime());
        }

        /** Check if an integer represents timeout */
        static bool isTimeout(const queue_e_t& v)
        {
            return v == _TIMEOUT_VALUE();
        }

        /** Yes, it is not a nice solution, but in this special case we need
         * direct access to the queue, because we test its full, empty, etc.
         * methods as well */
        queue_t& q()
        {
            return *_q;
        }

    private:
        /**
         * This is a code generator macro. It generates function objects to push
         * and pop a value. Those function objects are executed in the threads.
         *
         * Macro parameters:
         *
         *   __NAME__: the class name
         *   __CODE__: a piece of code that is executed first when the operator()
         *             is called. Note that the macro encapsulates nothing, it
         *             is used because the two wunction objects are really similar,
         *             and their code cannot be reused with regular C++ constructs.
         *
         * The functions store shared pointers to the queue and the valut that
         * must be pushed / was popped. It is necessary as the those objects
         * must refer to the same entity after copying the functions several times by the
         * threading subsystem, and they must ensure that there are no scope problems.
         */
#define _FUNCTOR_GENERATOR(_NAME_,_CODE_) \
			class _NAME_ : public ThreadFunction { \
			public: \
				_NAME_(const queue_p_t& q, const queue_e_t& v = _INIT_VALUE()) \
					: ThreadFunction(), _q(q), _v(new queue_e_t(v)) {} \
				_NAME_(const _NAME_& x) : ThreadFunction(x), _q(x._q), _v(x._v) {} \
			\
			virtual void operator() () { \
				_CODE_; \
				_finished(); \
			} \
			\
			queue_e_t getVal() const { \
				return *_v; \
			} \
		\
		private: \
			queue_p_t _q; \
			std::shared_ptr<queue_e_t> _v; \
		}

        /** Push a value to the queue. The value and the queue is set in the
         * constructor. */
        _FUNCTOR_GENERATOR(_PushFunc, _q->push(_v));
        /* Pop a value from the queue. The actual value van be obtained by
         * calling getVal() */
        _FUNCTOR_GENERATOR(_PopFunc, *_v = *(_q->pop()));
#undef _FUNCTOR_GENERATOR

        queue_p_t _q;
        boost::thread_group _thgrp;

        static const size_t _QUEUE_SIZE = 2;
        // Timeout for the blocking call.
        static Timeout& _WAIT_TIME()
        {
            static Timeout t(1, 0, 0);
            return t;
        }

        static queue_e_t _INIT_VALUE()
        {
            return queue_e_t(-2);
        };

        static queue_e_t _TIMEOUT_VALUE()
        {
            return queue_e_t(-1);
        };
    };
};
#if 0
/** Test if:
 * - The queue maintains the order of the elements
 * - properly reports if the queue is full or empty */
BOOST_FIXTURE_TEST_CASE (SynchronizedQueue_Test_elementOrder, SynchronizedQueueTest)
{
    _TestContext tc;
    BOOST_CHECK(tc.q().empty());
    BOOST_CHECK(!tc.q().full());
    BOOST_CHECK(tc.push(0));
    BOOST_CHECK(!tc.q().empty());
    BOOST_CHECK(!tc.q().full());
    BOOST_CHECK(tc.push(1));
    BOOST_CHECK(!tc.q().empty());
    BOOST_CHECK(tc.q().full());
    BOOST_CHECK_EQUAL(0, (int) tc.pop());
    BOOST_CHECK(!tc.q().empty());
    BOOST_CHECK(!tc.q().full());
    BOOST_CHECK_EQUAL(1, (int) tc.pop());
    BOOST_CHECK(tc.q().empty());
    BOOST_CHECK(!tc.q().full());
}

/** Test if the the pushung thread is blocked on a full queue */
BOOST_FIXTURE_TEST_CASE (SynchronizedQueue_Test_blockOnFull, SynchronizedQueueTest)
{
    _TestContext tc;
    BOOST_CHECK(tc.push(0));
    BOOST_CHECK(tc.push(1));
    // Ok, now pop a value to release the blocked thread
    bool pushres = tc.push(2);
    // We must do it before the assert, because in case of assertion, the blocked
    // thread never joins, and the execution is blocked on the tc destructor.
    tc.q().pop();
    BOOST_CHECK(!pushres);
}


/** Test if the popping thread is blocked on an empty queue */
BOOST_FIXTURE_TEST_CASE (SynchronizedQueue_Test_blockOnEmpty, SynchronizedQueueTest)
{
    _TestContext tc;
    BOOST_CHECK(tc.push(0));
    BOOST_CHECK(tc.push(1));
    BOOST_CHECK_EQUAL(0, (int) tc.pop());
    BOOST_CHECK_EQUAL(1, (int) tc.pop());
    bool res = tc.isTimeout(tc.pop());
    // Ok, now push a value to release the blocked thread to avoid resource leakage
    // We must do it before the assert, because in case of assertion, the blocked
    // thread never joins, and the execution is blocked on the tc destructor.
    tc.q().push(_TestContext::queue_t::element_type(new _TestContext::queue_e_t(2)));
    BOOST_CHECK(res);
}


/** Test if a popping blocked thread on an empty queue gets notified when
 * the queue receives an element */
BOOST_FIXTURE_TEST_CASE (SynchronizedQueue_Test_notifyIfEmptyGotElement, SynchronizedQueueTest)
{
    _TestContext tc;
    BOOST_CHECK(tc.push(0));
    BOOST_CHECK(tc.push(1));
    // This thread should block
    BOOST_CHECK(!tc.push(2));
    // Now we pop an element, the previous thread should wake up and push 2 in the queue
    BOOST_CHECK_EQUAL(0, (int) tc.pop());
    BOOST_CHECK_EQUAL(1, (int) tc.pop());
    BOOST_CHECK_EQUAL(2, (int) tc.pop());
}


/** Test if a pushing blocked thread on a full queue gets notified when
    the queue looses an element (somebody pops en element) */
BOOST_FIXTURE_TEST_CASE (SynchronizedQueue_Test_notifyIfFullLostElement, SynchronizedQueueTest)
{
    _TestContext tc;
    bool res = tc.isTimeout(tc.pop());
    BOOST_CHECK(res);
    BOOST_CHECK(tc.push(0));
    BOOST_CHECK(tc.push(1));
    BOOST_CHECK(!tc.isTimeout(tc.pop()));
    // It must block again as the queue got empty
    //BOOST_CHECK(tc.isTimeout(tc.pop()));
}

#endif // FTS3_COMPILE_WITH_UNITTESTS
#endif


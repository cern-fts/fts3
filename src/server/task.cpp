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

#include "task.h"

#ifdef FTS3_COMPILE_WITH_UNITTEST
#include "unittest/testsuite.h"
#endif // FTS3_COMPILE_WITH_UNITTESTS

/* ---------------------------------------------------------------------- */

FTS3_SERVER_NAMESPACE_START

#ifdef FTS3_COMPILE_WITH_UNITTEST

/** Test task operation */
class TestOp
{
public:

    /* ------------------------------------------------------------------ */

    typedef int value_type;

    /* ------------------------------------------------------------------ */

    enum enabled_value_type {INVALID_VALUE, VALUE_1, VALUE_2, VALUE_3};

    /* ------------------------------------------------------------------ */

    TestOp
    (
        value_type& ret, /* The external variable */
        int expected /* The value to be stored */
    )
        : _ret(ret), _exp(expected) {};

    /* ------------------------------------------------------------------ */

    void operator () ()
    {
        _ret = _exp;
    };

    /* ------------------------------------------------------------------ */

    static value_type val(enabled_value_type e)
    {
        return static_cast<value_type>(e);
    }

private:

    /* ------------------------------------------------------------------ */

    value_type& _ret;

    /* ------------------------------------------------------------------ */

    const value_type _exp;
};

/* ---------------------------------------------------------------------- */

/** The operation throws an unhandled exception */
class TestOpThrows
{
public:

    /* ------------------------------------------------------------------ */

    void operator () ()
    {
        throw exception_type(1);
    }

    /* ------------------------------------------------------------------ */

    typedef int exception_type;
};

/* ---------------------------------------------------------------------- */

BOOST_AUTO_TEST_CASE (Task_testExecutingOperation)
{
    TestOp::value_type val(TestOp::INVALID_VALUE);
    TestOp op(val, TestOp::VALUE_1);
    Task<TestOp> t(op);
    /* Check if the initial value is correct */
    BOOST_CHECK_EQUAL(TestOp::val(TestOp::INVALID_VALUE), val);
    t.execute();
    /* Test if after invoking the task operation, the value changed */
    BOOST_CHECK_EQUAL(TestOp::val(TestOp::VALUE_1), val);
}

/* ---------------------------------------------------------------------- */

BOOST_AUTO_TEST_CASE(Task_testCatchingUnhandledException)
{
    TestOpThrows op;
    Task<TestOpThrows> t(op);
    /* The operation throws an exception */
    t.execute();
}

#endif // FTS3_COMPILE_WITH_UNITTESTS

FTS3_SERVER_NAMESPACE_END


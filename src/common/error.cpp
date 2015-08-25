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

/** \file error.cpp Implementation of FTS3 error handling component. */

#include "error.h"

#ifdef FTS3_COMPILE_WITH_UNITTEST_NEW
#include "unittest/testsuite.h"
#include <boost/algorithm/string/find.hpp>
#endif // FTS3_COMPILE_WITH_UNITTESTS

namespace fts3 {
namespace common {

/* -------------------------------------------------------------------------- */

template<>
void Error<true>::_logSystemError()
{
    theLogger() << " (System reported: \"" << addErr << "\")";
}

/* -------------------------------------------------------------------------- */

void Err::log(const char* aFile, const char* aFunc, const int aLineNo)
{
    theLogger().newLog<Logger::type_traits::ERR>(aFile, aFunc, aLineNo) << _description();
    _logSystemError();
    theLogger() << commit;
}

/* -------------------------------------------------------------------------- */

const char* Err::what() const throw()
{
    return "Unknown error";
}

/* -------------------------------------------------------------------------- */

std::string Err_System::_description() const
{
    return _userDesc;
}

/* -------------------------------------------------------------------------- */

std::string Err_Custom::_description() const
{
    return _desc;
}

/* ========================================================================== */

#ifdef FTS3_COMPILE_WITH_UNITTEST_NEW

bool Common__Error_Macros_chekMessage (const Err_Custom&)
{
    return true;
}

/* -------------------------------------------------------------------------- */

BOOST_AUTO_TEST_SUITE( common )
BOOST_AUTO_TEST_SUITE(ErrorTest)

BOOST_AUTO_TEST_CASE (Common__Error_Macros)
{
    BOOST_CHECK_EXCEPTION
    (
        throw(Err_Custom("Error message")),
        Err_Custom,
        Common__Error_Macros_chekMessage
    );
}

BOOST_AUTO_TEST_SUITE_END()
BOOST_AUTO_TEST_SUITE_END()

#endif // FTS3_COMPILE_WITH_UNITTESTS

} // end namespace common
} // end namespace fts3


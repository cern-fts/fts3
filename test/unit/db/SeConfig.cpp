/*
 * Copyright (c) CERN 2017
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

#include <boost/test/unit_test_suite.hpp>
#include <boost/test/test_tools.hpp>
#include "db/generic/StorageConfig.h"

BOOST_AUTO_TEST_SUITE(db)

BOOST_AUTO_TEST_CASE (SeConfigMerge)
{
    StorageConfig target;
    target.inboundMaxActive = 60;

    StorageConfig src;
    src.udt = true;
    src.inboundMaxActive = 100;
    src.outboundMaxActive = 100;

    target.merge(src);

    BOOST_CHECK_EQUAL(60, target.inboundMaxActive);
    BOOST_CHECK_EQUAL(100, target.outboundMaxActive);
    BOOST_CHECK_EQUAL(true, target.udt.value);
    BOOST_CHECK(boost::indeterminate(target.ipv6));

}

BOOST_AUTO_TEST_SUITE_END()

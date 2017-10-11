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

#include "server/services/transfers/VoShares.h"

using namespace fts3::server;

BOOST_AUTO_TEST_SUITE(server)
BOOST_AUTO_TEST_SUITE(VoSharesTestSuite)

const Pair pair("mock://a", "mock://b");

/**
 * Try to schedule three VOs for the same pair. There is no available share
 * for dteam, so it should be inserted into unschedulable
 */
BOOST_AUTO_TEST_CASE (TestNoShare)
{
    std::vector<std::pair<std::string, unsigned>> vos{{"atlas", 0}, {"dteam", 10}, {"cms", 5}};
    std::map<std::string, double> weights;
    weights["atlas"] = 20;
    weights["cms"] = 20;

    std::vector<QueueId> unschedulable;
    boost::optional<QueueId> scheduled = selectQueueForPair(pair, vos, weights, unschedulable);

    BOOST_CHECK(static_cast<bool>(scheduled));

    BOOST_CHECK_EQUAL(pair.source, scheduled->sourceSe);
    BOOST_CHECK_EQUAL(pair.destination, scheduled->destSe);
    BOOST_CHECK_NE("dteam", scheduled->voName);
    BOOST_CHECK(scheduled->voName == "cms" || scheduled->voName == "atlas");

    BOOST_CHECK_EQUAL(1, unschedulable.size());
    BOOST_CHECK_EQUAL(pair.source, unschedulable[0].sourceSe);
    BOOST_CHECK_EQUAL(pair.destination, unschedulable[0].destSe);
    BOOST_CHECK_EQUAL("dteam", unschedulable[0].voName);
}

/**
 * Try to schedule three VOs for the same pair. There is no available share
 * for dteam, so it should be inserted into unschedulable
 */
BOOST_AUTO_TEST_CASE (TestNoShareSingle)
{
    std::vector<std::pair<std::string, unsigned>> vos{{"dteam", 0}};
    std::map<std::string, double> weights;
    weights["atlas"] = 20;
    weights["cms"] = 20;

    std::vector<QueueId> unschedulable;
    boost::optional<QueueId> scheduled = selectQueueForPair(pair, vos, weights, unschedulable);

    BOOST_CHECK(!scheduled);

    BOOST_CHECK_EQUAL(1, unschedulable.size());
    BOOST_CHECK_EQUAL(pair.source, unschedulable[0].sourceSe);
    BOOST_CHECK_EQUAL(pair.destination, unschedulable[0].destSe);
    BOOST_CHECK_EQUAL("dteam", unschedulable[0].voName);
}

/**
 * Same as before, but there is an explicit 'public' catch all with value 0
 */
BOOST_AUTO_TEST_CASE (TestNoSharePublic0)
{
    std::vector<std::pair<std::string, unsigned>> vos{{"atlas", 0}, {"dteam", 10}, {"cms", 5}};
    std::map<std::string, double> weights;
    weights["atlas"] = 20;
    weights["cms"] = 20;
    weights["public"] = 0;

    std::vector<QueueId> unschedulable;
    boost::optional<QueueId> scheduled = selectQueueForPair(pair, vos, weights, unschedulable);

    BOOST_CHECK(static_cast<bool>(scheduled));

    BOOST_CHECK_EQUAL(pair.source, scheduled->sourceSe);
    BOOST_CHECK_EQUAL(pair.destination, scheduled->destSe);
    BOOST_CHECK_NE("dteam", scheduled->voName);
    BOOST_CHECK(scheduled->voName == "cms" || scheduled->voName == "atlas");

    BOOST_CHECK_EQUAL(1, unschedulable.size());
    BOOST_CHECK_EQUAL(pair.source, unschedulable[0].sourceSe);
    BOOST_CHECK_EQUAL(pair.destination, unschedulable[0].destSe);
    BOOST_CHECK_EQUAL("dteam", unschedulable[0].voName);
}

/**
 * Catch-all public share, so all are schedulable
 */
BOOST_AUTO_TEST_CASE (TestPublicShare)
{
    std::vector<std::pair<std::string, unsigned>> vos{{"atlas", 0}, {"dteam", 10}, {"cms", 5}};
    std::map<std::string, double> weights;
    weights["atlas"] = 20;
    weights["cms"] = 20;
    weights["public"] = 2;

    std::vector<QueueId> unschedulable;
    boost::optional<QueueId> scheduled = selectQueueForPair(pair, vos, weights, unschedulable);

    BOOST_CHECK(static_cast<bool>(scheduled));

    BOOST_CHECK_EQUAL(pair.source, scheduled->sourceSe);
    BOOST_CHECK_EQUAL(pair.destination, scheduled->destSe);
    BOOST_CHECK(scheduled->voName == "cms" || scheduled->voName == "atlas" || scheduled->voName == "dteam");

    BOOST_CHECK_EQUAL(0, unschedulable.size());
}

/**
 * Empty VO share configuration, so fair-share
 */
BOOST_AUTO_TEST_CASE (TestPublicNoShareConfig)
{
    std::vector<std::pair<std::string, unsigned>> vos{{"atlas", 0}, {"dteam", 10}, {"cms", 5}};
    std::map<std::string, double> weights;

    std::vector<QueueId> unschedulable;
    boost::optional<QueueId> scheduled = selectQueueForPair(pair, vos, weights, unschedulable);

    BOOST_CHECK(static_cast<bool>(scheduled));

    BOOST_CHECK_EQUAL(pair.source, scheduled->sourceSe);
    BOOST_CHECK_EQUAL(pair.destination, scheduled->destSe);
    BOOST_CHECK(scheduled->voName == "cms" || scheduled->voName == "atlas" || scheduled->voName == "dteam");

    BOOST_CHECK_EQUAL(0, unschedulable.size());
}

/**
 * Give different weights, run multiple times.
 * Trivial check: at least the highest share runs more times
 */
BOOST_AUTO_TEST_CASE (TestShareRespected)
{
    const unsigned NRuns = 200;

    std::vector<std::pair<std::string, unsigned>> vos{{"atlas", 0}, {"dteam", 10}, {"cms", 5}, {"bitface", 4}};
    std::map<std::string, double> weights;
    weights["atlas"] = 80;
    weights["cms"]   = 15;
    weights["dteam"] = 5;

    std::map<std::string, int> count;

    for (unsigned i = 0; i < NRuns; ++i) {
        std::vector<QueueId> unschedulable;
        boost::optional<QueueId> scheduled = selectQueueForPair(pair, vos, weights, unschedulable);
        BOOST_CHECK_EQUAL(1, unschedulable.size());
        BOOST_CHECK(scheduled);
        count[scheduled->voName]++;
    }

    BOOST_CHECK_NE(0, count["atlas"]);
    BOOST_CHECK_NE(0, count["cms"]);
    BOOST_CHECK_NE(0, count["dteam"]);
    BOOST_CHECK_EQUAL(0, count["bitface"]);

    BOOST_CHECK_GT(count["atlas"], count["cms"]);
    BOOST_CHECK_GT(count["cms"], count["dteam"]);
}

BOOST_AUTO_TEST_SUITE_END()
BOOST_AUTO_TEST_SUITE_END()

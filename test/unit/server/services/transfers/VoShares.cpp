/*
 * Copyright (c) CERN 2024
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

#include <gtest/gtest.h>

#include "server/services/transfers/VoShares.h"

using namespace fts3::server;

const Pair pair("mock://a", "mock://b");

/**
 * Try to schedule three VOs for the same pair. There is no available share
 * for dteam, so it should be inserted into unschedulable
 */
TEST(VoSharesTest, TestNoShare) {
    std::vector<std::pair<std::string, unsigned>> vos{{"atlas", 0},
                                                      {"dteam", 10},
                                                      {"cms",   5}};
    std::map<std::string, double> weights;
    weights["atlas"] = 20;
    weights["cms"] = 20;

    std::vector<QueueId> unschedulable;
    boost::optional<QueueId> scheduled = selectQueueForPair(pair, vos, weights, unschedulable);

    EXPECT_TRUE(static_cast<bool>(scheduled));

    EXPECT_EQ(pair.source, scheduled->sourceSe);
    EXPECT_EQ(pair.destination, scheduled->destSe);
    EXPECT_NE("dteam", scheduled->voName);
    EXPECT_TRUE(scheduled->voName == "cms" || scheduled->voName == "atlas");

    EXPECT_EQ(1, unschedulable.size());
    EXPECT_EQ(pair.source, unschedulable[0].sourceSe);
    EXPECT_EQ(pair.destination, unschedulable[0].destSe);
    EXPECT_EQ("dteam", unschedulable[0].voName);
}

/**
 * Try to schedule three VOs for the same pair. There is no available share
 * for dteam, so it should be inserted into unschedulable
 */
TEST(VoSharesTest, TestNoShareSingle) {
    std::vector<std::pair<std::string, unsigned>> vos{{"dteam", 0}};
    std::map<std::string, double> weights;
    weights["atlas"] = 20;
    weights["cms"] = 20;

    std::vector<QueueId> unschedulable;
    boost::optional<QueueId> scheduled = selectQueueForPair(pair, vos, weights, unschedulable);

    EXPECT_TRUE(!scheduled);

    EXPECT_EQ(1, unschedulable.size());
    EXPECT_EQ(pair.source, unschedulable[0].sourceSe);
    EXPECT_EQ(pair.destination, unschedulable[0].destSe);
    EXPECT_EQ("dteam", unschedulable[0].voName);
}

/**
 * Same as before, but there is an explicit 'public' catch all with value 0
 */
TEST(VoSharesTest, TestNoSharePublic0) {
    std::vector<std::pair<std::string, unsigned>> vos{{"atlas", 0},
                                                      {"dteam", 10},
                                                      {"cms",   5}};
    std::map<std::string, double> weights;
    weights["atlas"] = 20;
    weights["cms"] = 20;
    weights["public"] = 0;

    std::vector<QueueId> unschedulable;
    boost::optional<QueueId> scheduled = selectQueueForPair(pair, vos, weights, unschedulable);

    EXPECT_TRUE(static_cast<bool>(scheduled));

    EXPECT_EQ(pair.source, scheduled->sourceSe);
    EXPECT_EQ(pair.destination, scheduled->destSe);
    EXPECT_NE("dteam", scheduled->voName);
    EXPECT_TRUE(scheduled->voName == "cms" || scheduled->voName == "atlas");

    EXPECT_EQ(1, unschedulable.size());
    EXPECT_EQ(pair.source, unschedulable[0].sourceSe);
    EXPECT_EQ(pair.destination, unschedulable[0].destSe);
    EXPECT_EQ("dteam", unschedulable[0].voName);
}

/**
 * Catch-all public share, so all are schedulable
 */
TEST(VoSharesTest, TestPublicShare) {
    std::vector<std::pair<std::string, unsigned>> vos{{"atlas", 0},
                                                      {"dteam", 10},
                                                      {"cms",   5}};
    std::map<std::string, double> weights;
    weights["atlas"] = 20;
    weights["cms"] = 20;
    weights["public"] = 2;

    std::vector<QueueId> unschedulable;
    boost::optional<QueueId> scheduled = selectQueueForPair(pair, vos, weights, unschedulable);

    EXPECT_TRUE(static_cast<bool>(scheduled));

    EXPECT_EQ(pair.source, scheduled->sourceSe);
    EXPECT_EQ(pair.destination, scheduled->destSe);
    EXPECT_TRUE(scheduled->voName == "cms" || scheduled->voName == "atlas" || scheduled->voName == "dteam");

    EXPECT_EQ(0, unschedulable.size());
}

/**
 * Empty VO share configuration, so fair-share
 */
TEST(VoSharesTest, TestPublicNoShareConfig) {
    std::vector<std::pair<std::string, unsigned>> vos{{"atlas", 0},
                                                      {"dteam", 10},
                                                      {"cms",   5}};
    std::map<std::string, double> weights;

    std::vector<QueueId> unschedulable;
    boost::optional<QueueId> scheduled = selectQueueForPair(pair, vos, weights, unschedulable);

    EXPECT_TRUE(static_cast<bool>(scheduled));

    EXPECT_EQ(pair.source, scheduled->sourceSe);
    EXPECT_EQ(pair.destination, scheduled->destSe);
    EXPECT_TRUE(scheduled->voName == "cms" || scheduled->voName == "atlas" || scheduled->voName == "dteam");

    EXPECT_EQ(0, unschedulable.size());
}

/**
 * Give different weights, run multiple times.
 * Trivial check: at least the highest share runs more times
 */
TEST(VoSharesTest, TestShareRespected) {
    const unsigned NRuns = 200;

    std::vector<std::pair<std::string, unsigned>> vos{{"atlas",   0},
                                                      {"dteam",   10},
                                                      {"cms",     5},
                                                      {"bitface", 4}};
    std::map<std::string, double> weights;
    weights["atlas"] = 80;
    weights["cms"] = 15;
    weights["dteam"] = 5;

    std::map<std::string, int> count;

    for (unsigned i = 0; i < NRuns; ++i) {
        std::vector<QueueId> unschedulable;
        boost::optional<QueueId> scheduled = selectQueueForPair(pair, vos, weights, unschedulable);
        EXPECT_EQ(1, unschedulable.size());
        EXPECT_TRUE(scheduled);
        count[scheduled->voName]++;
    }

    EXPECT_NE(0, count["atlas"]);
    EXPECT_NE(0, count["cms"]);
    EXPECT_NE(0, count["dteam"]);
    EXPECT_EQ(0, count["bitface"]);

    EXPECT_GT(count["atlas"], count["cms"]);
    EXPECT_GT(count["cms"], count["dteam"]);
}
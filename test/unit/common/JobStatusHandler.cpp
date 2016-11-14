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

#include <boost/test/unit_test_suite.hpp>
#include <boost/test/test_tools.hpp>

#include "common/JobStatusHandler.h"
#include "db/generic/JobStatus.h"

using fts3::common::JobStatusHandler;


BOOST_AUTO_TEST_SUITE(common)
BOOST_AUTO_TEST_SUITE(JobStatusHandlerTest)


BOOST_FIXTURE_TEST_CASE(JobStatusHandlerIsTransferFinished, JobStatusHandler)
{
    // stated indicating that the transfer has been finished:
    BOOST_CHECK(isTransferFinished(FTS3_STATUS_FINISHEDDIRTY));
    BOOST_CHECK(isTransferFinished(FTS3_STATUS_CANCELED));
    BOOST_CHECK(isTransferFinished(FTS3_STATUS_UNKNOWN));
    BOOST_CHECK(isTransferFinished(FTS3_STATUS_FAILED));
    BOOST_CHECK(isTransferFinished(FTS3_STATUS_FINISHED));

    // states indicating that the transfer is still being carried out
    BOOST_CHECK(!isTransferFinished(FTS3_STATUS_SUBMITTED));
    BOOST_CHECK(!isTransferFinished(FTS3_STATUS_READY));
    BOOST_CHECK(!isTransferFinished(FTS3_STATUS_ACTIVE));
    BOOST_CHECK(!isTransferFinished(FTS3_STATUS_STAGING));
    BOOST_CHECK(!isTransferFinished(FTS3_STATUS_NOT_USED));
}


BOOST_FIXTURE_TEST_CASE(JobStatusHandlerIsStatusValid, JobStatusHandler)
{
    BOOST_CHECK(isStatusValid(FTS3_STATUS_FINISHEDDIRTY));
    BOOST_CHECK(isStatusValid(FTS3_STATUS_CANCELED));
    BOOST_CHECK(isStatusValid(FTS3_STATUS_UNKNOWN));
    BOOST_CHECK(isStatusValid(FTS3_STATUS_FAILED));
    BOOST_CHECK(isStatusValid(FTS3_STATUS_FINISHED));
    BOOST_CHECK(isStatusValid(FTS3_STATUS_SUBMITTED));
    BOOST_CHECK(isStatusValid(FTS3_STATUS_READY));
    BOOST_CHECK(isStatusValid(FTS3_STATUS_ACTIVE));
    BOOST_CHECK(isStatusValid(FTS3_STATUS_STAGING));
    BOOST_CHECK(isStatusValid(FTS3_STATUS_NOT_USED));

    // some random string
    BOOST_CHECK(!isStatusValid("ahjcakjgkas"));
}


BOOST_FIXTURE_TEST_CASE(JobStatusHandler_countInState, JobStatusHandler)
{
    std::vector<FileTransferStatus> statuses(7);
    statuses[0].fileState = FTS3_STATUS_ACTIVE;
    statuses[0].fileIndex = 0;
    statuses[1].fileState = FTS3_STATUS_ACTIVE;
    statuses[1].fileIndex = 0;
    statuses[2].fileState = FTS3_STATUS_ACTIVE;
    statuses[2].fileIndex = 1;
    statuses[3].fileState = FTS3_STATUS_ACTIVE;
    statuses[3].fileIndex = 1;
    statuses[4].fileState = FTS3_STATUS_ACTIVE;
    statuses[4].fileIndex = 2;
    statuses[5].fileState = FTS3_STATUS_CANCELED;
    statuses[5].fileIndex = 3;
    statuses[6].fileState = FTS3_STATUS_CANCELED;
    statuses[6].fileIndex = 3;

    BOOST_CHECK_EQUAL(countInState(FTS3_STATUS_ACTIVE, statuses), 3);
    BOOST_CHECK_EQUAL(countInState(FTS3_STATUS_CANCELED, statuses), 1);
    BOOST_CHECK_EQUAL(countInState(FTS3_STATUS_FAILED, statuses), 0);
}

BOOST_AUTO_TEST_SUITE_END()
BOOST_AUTO_TEST_SUITE_END()

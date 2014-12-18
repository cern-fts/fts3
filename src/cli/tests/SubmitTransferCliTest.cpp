/*
 *	Copyright notice:
 *	Copyright © Members of the EMI Collaboration, 2010.
 *
 *	See www.eu-emi.eu for details on the copyright holders
 *
 *	Licensed under the Apache License, Version 2.0 (the "License");
 *	you may not use this file except in compliance with the License.
 *	You may obtain a copy of the License at
 *
 *		http://www.apache.org/licenses/LICENSE-2.0
 *
 *	Unless required by applicable law or agreed to in writing, software
 *	distributed under the License is distributed on an "AS IS" BASIS,
 *	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *	See the License for the specific language governing permissions and
 *	limitations under the License.
 *
 * SubmitTransferCliTest.cpp
 *
 *  Created on: Mar 19, 2012
 *      Author: Michal Simon
 */

#ifdef FTS3_COMPILE_WITH_UNITTEST_NEW

#include "ui/SubmitTransferCli.h"
#include "exception/bad_option.h"

#include "unittest/testsuite.h"

#include "common/JobParameterHandler.h"

#include <fstream>
#include <iostream>
#include <memory>

using namespace fts3::cli;
using namespace fts3::common;


BOOST_AUTO_TEST_SUITE( cli )
BOOST_AUTO_TEST_SUITE(SubmitTransferCliTest)

BOOST_AUTO_TEST_CASE (SubmitTransferCli_bulk_submission)
{

    // creat a tmeporary file with bulk-job description
    std::fstream fs("/tmp/bulk", std::fstream::out);
    if (!fs)
        {
            std::cout << "It was not possible to carry out the bulk-job test!" << std::endl;
            return;
        }

    fs << "srm://source1/file.in srm://destination1/file.out Alg:check1" << std::endl;
    fs << "srm://source2/file.in srm://destination2/file.out" << std::endl;

    fs.flush();
    fs.close();

    // argument vector
    char* av[] =
    {
        "prog_name",
        "-s",
        "https://fts3-server:8080",
        "-f",
        "/tmp/bulk"
    };

    // argument count
    int ac = 5;

    std::unique_ptr<SubmitTransferCli> cli (new SubmitTransferCli);
    cli->parse(ac, av);


    cli->validate();

    BOOST_CHECK(cli->useCheckSum());

    std::vector<File> files = cli->getFiles();

    BOOST_CHECK_EQUAL(files.size(), 2);

    BOOST_CHECK_EQUAL(files[0].sources[0], "srm://source1/file.in");
    BOOST_CHECK_EQUAL(files[0].destinations[0], "srm://destination1/file.out");
    BOOST_CHECK_EQUAL(files[0].checksums[0], "Alg:check1");

    BOOST_CHECK_EQUAL(files[1].sources[0], "srm://source2/file.in");
    BOOST_CHECK_EQUAL(files[1].destinations[0], "srm://destination2/file.out");
    BOOST_CHECK(files[1].checksums.empty());
}

BOOST_AUTO_TEST_CASE (SubmitTransferCli_other_options)
{
    // has to be const otherwise is deprecated
    char* av[] =
    {
        "prog_name",
        "-s",
        "https://fts3-server:8080",
        "srm://src/file.in",
        "srm://dst/file.out",
        "-b",
        "-e",
        "1234"
    };

    // argument count
    int ac = 8;

    std::unique_ptr<SubmitTransferCli> cli (new SubmitTransferCli);
    cli->parse(ac, av);
    cli->validate();

    BOOST_CHECK(cli->isBlocking());
    BOOST_CHECK(cli->getExpirationTime() == 1234);
}

BOOST_AUTO_TEST_CASE (SubmitTransferCli_submission_no_job)
{

    // has to be const otherwise is deprecated
    char* av[] =
    {
        "prog_name",
        "-s",
        "https://fts3-server:8080",
    };

    // argument count
    int ac = 3;

    std::unique_ptr<SubmitTransferCli> cli (new SubmitTransferCli);
    cli->parse(ac, av);
    BOOST_CHECK_THROW(cli->validate(), cli_exception);
    BOOST_CHECK_THROW(cli->getFiles(), bad_option);
}

BOOST_AUTO_TEST_CASE (SubmitTransferCli_submission_no_checksum)
{

    // has to be const otherwise is deprecated
    char* av[] =
    {
        "prog_name",
        "-s",
        "https://fts3-server:8080",
        "srm://source/file.in",
        "srm://destination/file.out"
    };

    // argument count
    int ac = 5;

    std::unique_ptr<SubmitTransferCli> cli (new SubmitTransferCli);
    cli->parse(ac, av);
    cli->validate();

    BOOST_CHECK_EQUAL(cli->getSource(), "srm://source/file.in");
    BOOST_CHECK_EQUAL(cli->getDestination(), "srm://destination/file.out");
    BOOST_CHECK(!cli->useCheckSum());

    std::vector<File> files = cli->getFiles();

    BOOST_CHECK_EQUAL(files.size(), 1);
    BOOST_CHECK_EQUAL(files[0].sources[0], "srm://source/file.in");
    BOOST_CHECK_EQUAL(files[0].destinations[0], "srm://destination/file.out");
}

BOOST_AUTO_TEST_CASE (SubmitTransferCli_submission_with_checksum)
{

    // has to be const otherwise is deprecated
    char* av[] =
    {
        "prog_name",
        "-s",
        "https://fts3-server:8080",
        "srm://source/file.in",
        "srm://destination/file.out",
        "ALGORITHM:1234af"
    };
    // argument count
    int ac = 6;

    std::unique_ptr<SubmitTransferCli> cli (new SubmitTransferCli);
    cli->parse(ac, av);
    cli->validate();

    BOOST_CHECK(cli->getSource().compare("srm://source/file.in") == 0);
    BOOST_CHECK(cli->getDestination().compare("srm://destination/file.out") == 0);
    BOOST_CHECK(cli->useCheckSum());

    std::vector<File> files = cli->getFiles();

    BOOST_CHECK_EQUAL(files.size(), 1);
    BOOST_CHECK_EQUAL(files[0].sources[0], "srm://source/file.in");
    BOOST_CHECK_EQUAL(files[0].destinations[0], "srm://destination/file.out");
    BOOST_CHECK_EQUAL(files[0].checksums[0], "ALGORITHM:1234af");
}

BOOST_FIXTURE_TEST_CASE (SubmitTransferCli_parameters, SubmitTransferCli)
{

    // has to be const otherwise is deprecated
    char* av[] =
    {
        "prog_name",
        "-s",
        "https://fts3-server:8080",
        "-o",
        "-K",
        "-g", "gparam",
        "-I", "id",
        "-t", "dest-token",
        "-S", "source-token",
        "--copy-pin-lifetime", "123",
        "srm://src/file.in",
        "srm://dst/file.out"
    };
    // argument count
    int ac = 15;

    std::unique_ptr<SubmitTransferCli> cli (new SubmitTransferCli);
    cli->parse(ac, av);

    try
        {
            cli->validate();
        }
    catch(cli_exception & ex)
        {
            std::string s1, s2;
            s1 = ex.what();
            s2 = ex.what();
        }

    std::map<std::string, std::string> params = cli->getParams();

    BOOST_CHECK_EQUAL(params.at(JobParameterHandler::CHECKSUM_METHOD), "compare");
    BOOST_CHECK_EQUAL(params.at(JobParameterHandler::OVERWRITEFLAG), "Y");
    BOOST_CHECK_EQUAL(params.at(JobParameterHandler::GRIDFTP), "gparam");
    BOOST_CHECK_EQUAL(params.at(JobParameterHandler::DELEGATIONID), "id");
    BOOST_CHECK_EQUAL(params.at(JobParameterHandler::SPACETOKEN), "dest-token");
    BOOST_CHECK_EQUAL(params.at(JobParameterHandler::SPACETOKEN_SOURCE), "source-token");
    BOOST_CHECK_EQUAL(params.at(JobParameterHandler::COPY_PIN_LIFETIME), "123");
}

BOOST_AUTO_TEST_SUITE_END()
BOOST_AUTO_TEST_SUITE_END()

#endif // FTS3_COMPILE_WITH_UNITTESTS

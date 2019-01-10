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

#include "cli/ui/SubmitTransferCli.h"
#include "cli/JobParameterHandler.h"
#include <boost/optional/optional_io.hpp>
#include <fstream>

using fts3::cli::SubmitTransferCli;
using fts3::cli::File;
using fts3::cli::bad_option;
using fts3::cli::cli_exception;
using fts3::cli::JobParameterHandler;


BOOST_AUTO_TEST_SUITE(cli)
BOOST_AUTO_TEST_SUITE(SubmitTransferCliTest)


BOOST_AUTO_TEST_CASE (SubmitTransferCli_bulk_submission)
{
    // create a temporary file with bulk-job description
    std::fstream fs("/tmp/bulk", std::fstream::out);
    if (!fs) {
        std::cout << "It was not possible to carry out the bulk-job test!" << std::endl;
        return;
    }

    fs << "srm://source1/file.in srm://destination1/file.out Alg:check1" << std::endl;
    fs << "srm://source2/file.in srm://destination2/file.out" << std::endl;

    fs.flush();
    fs.close();

    // argument vector
    std::vector<const char*> argv {
        "prog_name",
        "-s",
        "https://fts3-server:8080",
        "-f",
        "/tmp/bulk"
    };

    SubmitTransferCli cli;
    cli.parse(argv.size(), (char**)argv.data());
    cli.validate();

    BOOST_CHECK(cli.useCheckSum());

    std::vector<File> files = cli.getFiles();

    BOOST_CHECK_EQUAL(files.size(), 2);

    BOOST_CHECK_EQUAL(files[0].sources[0], "srm://source1/file.in");
    BOOST_CHECK_EQUAL(files[0].destinations[0], "srm://destination1/file.out");
    boost::optional<std::string> checksum = std::string("Alg:check1");
    BOOST_CHECK_EQUAL(files[0].checksum, checksum);

    BOOST_CHECK_EQUAL(files[1].sources[0], "srm://source2/file.in");
    BOOST_CHECK_EQUAL(files[1].destinations[0], "srm://destination2/file.out");
    BOOST_CHECK_EQUAL(files[1].checksum, boost::none);
}


BOOST_AUTO_TEST_CASE (SubmitTransferCli_other_options)
{
    std::vector<const char*> argv {
        "prog_name",
        "-s",
        "https://fts3-server:8080",
        "srm://src/file.in",
        "srm://dst/file.out",
        "-b",
        "-e",
        "1234"
    };

    SubmitTransferCli cli;
    cli.parse(argv.size(), (char**)argv.data());
    cli.validate();

    BOOST_CHECK(cli.isBlocking());
    BOOST_CHECK(cli.getExpirationTime() == 1234);
}


BOOST_AUTO_TEST_CASE (SubmitTransferCli_submission_no_job)
{
    std::vector<const char*> argv {
        "prog_name",
        "-s",
        "https://fts3-server:8080",
    };

    SubmitTransferCli cli;
    cli.parse(argv.size(), (char**)argv.data());

    BOOST_CHECK_THROW(cli.validate(), cli_exception);
    BOOST_CHECK_THROW(cli.getFiles(), bad_option);
}


BOOST_AUTO_TEST_CASE (SubmitTransferCli_submission_no_checksum)
{
    std::vector<const char*> argv {
        "prog_name",
        "-s",
        "https://fts3-server:8080",
        "srm://source/file.in",
        "srm://destination/file.out"
    };

    SubmitTransferCli cli;
    cli.parse(argv.size(), (char**)argv.data());
    cli.validate();

    BOOST_CHECK_EQUAL(cli.getSource(), "srm://source/file.in");
    BOOST_CHECK_EQUAL(cli.getDestination(), "srm://destination/file.out");
    BOOST_CHECK(!cli.useCheckSum());

    std::vector<File> files = cli.getFiles();

    BOOST_CHECK_EQUAL(files.size(), 1);
    BOOST_CHECK_EQUAL(files[0].sources[0], "srm://source/file.in");
    BOOST_CHECK_EQUAL(files[0].destinations[0], "srm://destination/file.out");
}


BOOST_AUTO_TEST_CASE (SubmitTransferCli_submission_with_checksum)
{
    std::vector<const char*> argv {
        "prog_name",
        "-s",
        "https://fts3-server:8080",
        "srm://source/file.in",
        "srm://destination/file.out",
        "ALGORITHM:1234af"
    };

    SubmitTransferCli cli;
    cli.parse(argv.size(), (char**)argv.data());
    cli.validate();

    BOOST_CHECK(cli.getSource().compare("srm://source/file.in") == 0);
    BOOST_CHECK(cli.getDestination().compare("srm://destination/file.out") == 0);
    BOOST_CHECK(cli.useCheckSum());

    std::vector<File> files = cli.getFiles();

    BOOST_CHECK_EQUAL(files.size(), 1);
    BOOST_CHECK_EQUAL(files[0].sources[0], "srm://source/file.in");
    boost::optional<std::string> checksum = std::string("ALGORITHM:1234af");
    BOOST_CHECK_EQUAL(files[0].destinations[0], "srm://destination/file.out");
    BOOST_CHECK_EQUAL(files[0].checksum, checksum);
}


BOOST_FIXTURE_TEST_CASE (SubmitTransferCli_parameters, SubmitTransferCli)
{
    std::vector<const char*> argv {
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

    SubmitTransferCli cli;
    cli.parse(argv.size(), (char**)argv.data());


    std::map<std::string, std::string> params = cli.getParams();

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

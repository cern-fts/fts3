/*
 * Copyright (c) CERN 2023
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
#include <json/json.h>
#include <cstdint>

#include "UrlCopyFixture.h"

BOOST_AUTO_TEST_SUITE(dest_file_report)


BOOST_FIXTURE_TEST_CASE (simpleDestFileReport, UrlCopyFixture)
{
    Transfer original;
    original.source = Uri::parse("mock://host/path?size=10");
    original.destination = Uri::parse("mock://host/path?size_pre=10&size_post=10&time=2"
                                      "&checksum=abc123ab&user.status=NEARLINE");
    opts.transfers.push_back(original);

    // Ask for destination file report
    opts.dstFileReport = true;

    UrlCopyProcess proc(opts, *this);
    proc.run();

    BOOST_CHECK_EQUAL(startMsgs.size(), 1);
    BOOST_CHECK_EQUAL(completedMsgs.size(), 1);

    Transfer &c = completedMsgs.front();
    BOOST_CHECK_NE(c.error.get(), (void*)NULL);
    BOOST_CHECK_EQUAL(c.error->code(), EEXIST);

    BOOST_CHECK_NE(c.fileMetadata, "");

    // Parse metadata containing the destination file report
    Json::Value metadata;
    std::istringstream valueStream(c.fileMetadata);
    valueStream >> metadata;

    BOOST_CHECK_EQUAL(metadata.isMember("dst_file"), true);

    Json::Value dst_file = metadata["dst_file"];

    BOOST_CHECK_EQUAL(dst_file["file_size"].asUInt64(), 10);
    BOOST_CHECK_EQUAL(dst_file["checksum_type"], "ADLER32");
    BOOST_CHECK_EQUAL(dst_file["checksum_value"], "abc123ab");
    BOOST_CHECK_EQUAL(dst_file["file_on_disk"], false);
    BOOST_CHECK_EQUAL(dst_file["file_on_tape"], true);

}


BOOST_FIXTURE_TEST_CASE (overwriteEnabled, UrlCopyFixture) {
    Transfer original;
    original.source = Uri::parse("mock://host/path?size=10&checksum=abc123ab");
    original.destination = Uri::parse("mock://host/path?size_pre=10&size_post=10&time=2"
                                      "&checksum=abc123ab&user.status=NEARLINE");
    opts.transfers.push_back(original);

    // Ask for destination file report
    opts.dstFileReport = true;
    opts.overwrite = true; // Because overwrite is enabled later we should not receive a dest file report

    UrlCopyProcess proc(opts, *this);
    proc.run();

    BOOST_CHECK_EQUAL(startMsgs.size(), 1);
    BOOST_CHECK_EQUAL(completedMsgs.size(), 1);
    BOOST_CHECK_EQUAL(protoMsgs.size(), 1);

    Transfer &c = completedMsgs.front();
    // No error received because overwrite is enabled
    BOOST_CHECK_EQUAL(c.error.get(), (void*)NULL);
    BOOST_CHECK_EQUAL(c.fileSize, 10);

    // File metadata should continue empty as dest file report is not created
    BOOST_CHECK_EQUAL(c.fileMetadata, "");
}


BOOST_FIXTURE_TEST_CASE (fileOnlineAndNearline, UrlCopyFixture)
{
    Transfer original;
    original.source = Uri::parse("mock://host/path?size=10");
    original.destination = Uri::parse("mock://host/path?size_pre=10&time=2&user.status=ONLINE_AND_NEARLINE");
    opts.transfers.push_back(original);

    // Ask for destination file report
    opts.dstFileReport = true;

    UrlCopyProcess proc(opts, *this);
    proc.run();

    BOOST_CHECK_EQUAL(startMsgs.size(), 1);
    BOOST_CHECK_EQUAL(completedMsgs.size(), 1);

    Transfer &c = completedMsgs.front();
    BOOST_CHECK_NE(c.error.get(), (void*)NULL);
    BOOST_CHECK_EQUAL(c.error->code(), EEXIST);

    BOOST_CHECK_NE(c.fileMetadata, "");

    // Parse metadata containing the destination file report
    Json::Value metadata;
    std::istringstream valueStream(c.fileMetadata);
    valueStream >> metadata;

    BOOST_CHECK_EQUAL(metadata.isMember("dst_file"), true);

    Json::Value dst_file = metadata["dst_file"];

    BOOST_CHECK_EQUAL(dst_file["file_on_disk"], true);
    BOOST_CHECK_EQUAL(dst_file["file_on_tape"], true);
}


BOOST_FIXTURE_TEST_CASE (fileOnline, UrlCopyFixture)
{
    Transfer original;
    original.source = Uri::parse("mock://host/path?size=10");
    original.destination = Uri::parse("mock://host/path?size_pre=10&time=2&user.status=ONLINE");
    opts.transfers.push_back(original);

    // Ask for destination file report
    opts.dstFileReport = true;

    UrlCopyProcess proc(opts, *this);
    proc.run();

    BOOST_CHECK_EQUAL(startMsgs.size(), 1);
    BOOST_CHECK_EQUAL(completedMsgs.size(), 1);

    Transfer &c = completedMsgs.front();
    BOOST_CHECK_NE(c.error.get(), (void*)NULL);
    BOOST_CHECK_EQUAL(c.error->code(), EEXIST);

    BOOST_CHECK_NE(c.fileMetadata, "");

    // Parse metadata containing the destination file report
    Json::Value metadata;
    std::istringstream valueStream(c.fileMetadata);
    valueStream >> metadata;

    BOOST_CHECK_EQUAL(metadata.isMember("dst_file"), true);

    Json::Value dst_file = metadata["dst_file"];

    BOOST_CHECK_EQUAL(dst_file["file_on_disk"], true);
    BOOST_CHECK_EQUAL(dst_file["file_on_tape"], false);
}


BOOST_FIXTURE_TEST_CASE (fileNearline, UrlCopyFixture)
{
    Transfer original;
    original.source = Uri::parse("mock://host/path?size=10");
    original.destination = Uri::parse("mock://host/path?size_pre=10&time=2&user.status=NEARLINE");
    opts.transfers.push_back(original);

    // Ask for destination file report
    opts.dstFileReport = true;

    UrlCopyProcess proc(opts, *this);
    proc.run();

    BOOST_CHECK_EQUAL(startMsgs.size(), 1);
    BOOST_CHECK_EQUAL(completedMsgs.size(), 1);

    Transfer &c = completedMsgs.front();
    BOOST_CHECK_NE(c.error.get(), (void*)NULL);
    BOOST_CHECK_EQUAL(c.error->code(), EEXIST);

    BOOST_CHECK_NE(c.fileMetadata, "");

    // Parse metadata containing the destination file report
    Json::Value metadata;
    std::istringstream valueStream(c.fileMetadata);
    valueStream >> metadata;

    BOOST_CHECK_EQUAL(metadata.isMember("dst_file"), true);

    Json::Value dst_file = metadata["dst_file"];

    BOOST_CHECK_EQUAL(dst_file["file_on_disk"], false);
    BOOST_CHECK_EQUAL(dst_file["file_on_tape"], true);
}


BOOST_FIXTURE_TEST_CASE (invalidStatus, UrlCopyFixture)
{
    Transfer original;
    original.source = Uri::parse("mock://host/path?size=10");
    original.destination = Uri::parse("mock://host/path?size_pre=10&time=2&user.status=INVALID");
    opts.transfers.push_back(original);

    // Ask for destination file report
    opts.dstFileReport = true;

    UrlCopyProcess proc(opts, *this);
    proc.run();

    BOOST_CHECK_EQUAL(startMsgs.size(), 1);
    BOOST_CHECK_EQUAL(completedMsgs.size(), 1);

    Transfer &c = completedMsgs.front();
    BOOST_CHECK_NE(c.error.get(), (void*)NULL);
    BOOST_CHECK_EQUAL(c.error->code(), EEXIST);

    BOOST_CHECK_NE(c.fileMetadata, "");

    // Parse metadata containing the destination file report
    Json::Value metadata;
    std::istringstream valueStream(c.fileMetadata);
    valueStream >> metadata;

    BOOST_CHECK_EQUAL(metadata.isMember("dst_file"), true);

    Json::Value dst_file = metadata["dst_file"];

    BOOST_CHECK_EQUAL(dst_file["file_on_disk"], false);
    BOOST_CHECK_EQUAL(dst_file["file_on_tape"], false);
}


BOOST_FIXTURE_TEST_CASE (md5Checksum, UrlCopyFixture)
{
    Transfer original;
    original.source = Uri::parse("mock://host/path?size=10");
    original.destination = Uri::parse("mock://host/path?size_pre=10&time=2&user.status=ONLINE&checksum=abc123ab");
    original.checksumAlgorithm = "MD5";
    opts.transfers.push_back(original);

    // Ask for destination file report
    opts.dstFileReport = true;

    UrlCopyProcess proc(opts, *this);
    proc.run();

    BOOST_CHECK_EQUAL(startMsgs.size(), 1);
    BOOST_CHECK_EQUAL(completedMsgs.size(), 1);

    Transfer &c = completedMsgs.front();
    BOOST_CHECK_NE(c.error.get(), (void*)NULL);
    BOOST_CHECK_EQUAL(c.error->code(), EEXIST);

    BOOST_CHECK_NE(c.fileMetadata, "");

    // Parse metadata containing the destination file report
    Json::Value metadata;
    std::istringstream valueStream(c.fileMetadata);
    valueStream >> metadata;

    BOOST_CHECK_EQUAL(metadata.isMember("dst_file"), true);

    Json::Value dst_file = metadata["dst_file"];

    BOOST_CHECK_EQUAL(dst_file["checksum_type"], "MD5");
    BOOST_CHECK_EQUAL(dst_file["checksum_value"], "abc123ab");
}


BOOST_FIXTURE_TEST_CASE (maxFileSize, UrlCopyFixture)
{
    Transfer original;
    uint64_t max_file_size = UINT64_MAX;

    std::stringstream source_url;
    source_url << "mock://host/path?size=" << max_file_size;

    std::stringstream dest_url;
    dest_url << "mock://host/path?time=2&user.status=ONLINE&checksum=abc123ab&size_pre=" << max_file_size << "&size_post=" << max_file_size;

    original.source = Uri::parse(source_url.str());
    original.destination = Uri::parse(dest_url.str());

    opts.transfers.push_back(original);

    // Ask for destination file report
    opts.dstFileReport = true;

    UrlCopyProcess proc(opts, *this);
    proc.run();

    BOOST_CHECK_EQUAL(startMsgs.size(), 1);
    BOOST_CHECK_EQUAL(completedMsgs.size(), 1);

    Transfer &c = completedMsgs.front();
    BOOST_CHECK_NE(c.error.get(), (void*)NULL);
    BOOST_CHECK_EQUAL(c.error->code(), EEXIST);

    BOOST_CHECK_NE(c.fileMetadata, "");

    // Parse metadata containing the destination file report
    Json::Value metadata;
    std::istringstream valueStream(c.fileMetadata);
    valueStream >> metadata;

    BOOST_CHECK_EQUAL(metadata.isMember("dst_file"), true);

    Json::Value dst_file = metadata["dst_file"];

    // Make sure that writing/reading from/to the JSON does not corrupt the value of file_size
    BOOST_CHECK_EQUAL(dst_file["file_size"].asUInt64(), max_file_size);
}


BOOST_FIXTURE_TEST_CASE (fileMetadataExists, UrlCopyFixture)
{
    Transfer original;
    original.source = Uri::parse("mock://host/path?size=10");
    original.destination = Uri::parse("mock://host/path?size_pre=10&size_post=10&time=2&user.status=NEARLINE");
    std::string test_metadata = "This is a test";
    // The following metadata will later be put into a JSON. The key that will contain this value is file_metadata
    original.fileMetadata = test_metadata;
    opts.transfers.push_back(original);

    // Ask for destination file report
    opts.dstFileReport = true;

    UrlCopyProcess proc(opts, *this);
    proc.run();

    BOOST_CHECK_EQUAL(startMsgs.size(), 1);
    BOOST_CHECK_EQUAL(completedMsgs.size(), 1);

    Transfer &c = completedMsgs.front();
    BOOST_CHECK_NE(c.error.get(), (void*)NULL);
    BOOST_CHECK_EQUAL(c.error->code(), EEXIST);

    BOOST_CHECK_NE(c.fileMetadata, "");

    // Parse metadata containing the destination file report
    Json::Value metadata;
    std::istringstream valueStream(c.fileMetadata);
    valueStream >> metadata;

    BOOST_CHECK_EQUAL(metadata.isMember("dst_file"), true);
    BOOST_CHECK_EQUAL(metadata.isMember("file_metadata"), true);

    BOOST_CHECK_EQUAL(metadata["file_metadata"], test_metadata);
}


BOOST_FIXTURE_TEST_CASE (fileMetadataExistsAsJSON, UrlCopyFixture)
{
    Transfer original;
    original.source = Uri::parse("mock://host/path?size=10");
    original.destination = Uri::parse("mock://host/path?size_pre=10&size_post=10&time=2&user.status=NEARLINE");

    // Create a JSON object and send it as the file metadata
    Json::Value output;
    output["activity"] = "Test";
    output["priority"] = 5;

    std::stringstream test_metadata;
    test_metadata << output;

    // Because the metadata sent to the url-copy is already a valid JSON,
    // the format must not be changed when the dest file report is received
    original.fileMetadata = test_metadata.str();

    opts.transfers.push_back(original);

    // Ask for destination file report
    opts.dstFileReport = true;

    UrlCopyProcess proc(opts, *this);
    proc.run();

    BOOST_CHECK_EQUAL(startMsgs.size(), 1);
    BOOST_CHECK_EQUAL(completedMsgs.size(), 1);

    Transfer &c = completedMsgs.front();
    BOOST_CHECK_NE(c.error.get(), (void*)NULL);
    BOOST_CHECK_EQUAL(c.error->code(), EEXIST);

    BOOST_CHECK_NE(c.fileMetadata, "");

    // Parse metadata containing the destination file report
    Json::Value metadata;
    std::istringstream valueStream(c.fileMetadata);
    valueStream >> metadata;

    BOOST_CHECK_EQUAL(metadata.isMember("dst_file"), true);
    BOOST_CHECK_EQUAL(metadata.isMember("activity"), true);
    BOOST_CHECK_EQUAL(metadata.isMember("priority"), true);

    BOOST_CHECK_EQUAL(metadata["activity"], "Test");
    BOOST_CHECK_EQUAL(metadata["priority"].asUInt(), 5);
}


BOOST_AUTO_TEST_SUITE_END()

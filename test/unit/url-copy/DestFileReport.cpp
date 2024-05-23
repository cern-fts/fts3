/*
 * Copyright (c) CERN 2024
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

#include <json/json.h>
#include <cstdint>

#include "UrlCopyFixture.h"


TEST_F(UrlCopyFixture, SimpleDestFileReport) {
    Transfer original;
    original.source = Uri::parse("mock://host/path?size=10");
    original.destination = Uri::parse("mock://host/path?size_pre=10&size_post=10&time=2"
                                      "&checksum=abc123ab&user.status=NEARLINE");
    opts.transfers.push_back(original);

    // Ask for destination file report
    opts.dstFileReport = true;

    UrlCopyProcess proc(opts, *this);
    proc.run();

    EXPECT_EQ(startMsgs.size(), 1);
    EXPECT_EQ(completedMsgs.size(), 1);

    Transfer &c = completedMsgs.front();
    EXPECT_NE(c.error.get(), (void *) NULL);
    EXPECT_EQ(c.error->code(), EEXIST);

    EXPECT_NE(c.fileMetadata, "");

    // Parse metadata containing the destination file report
    Json::Value metadata;
    std::istringstream valueStream(c.fileMetadata);
    valueStream >> metadata;

    EXPECT_EQ(metadata.isMember("dst_file"), true);

    Json::Value dst_file = metadata["dst_file"];

    EXPECT_EQ(dst_file["file_size"].asUInt64(), 10);
    EXPECT_EQ(dst_file["checksum_type"], "ADLER32");
    EXPECT_EQ(dst_file["checksum_value"], "abc123ab");
    EXPECT_EQ(dst_file["file_on_disk"], false);
    EXPECT_EQ(dst_file["file_on_tape"], true);
}

TEST_F (UrlCopyFixture, overwriteEnabled) {
    Transfer original;
    original.source = Uri::parse("mock://host/path?size=10");
    original.destination = Uri::parse("mock://host/path?size_pre=10&size_post=10&time=2"
                                      "&checksum=abc123ab&user.status=NEARLINE");
    opts.transfers.push_back(original);

    // Ask for destination file report
    opts.dstFileReport = true;
    opts.overwrite = true; // Because overwrite is enabled later we should not receive a dest file report

    UrlCopyProcess proc(opts, *this);
    proc.run();

    EXPECT_EQ(startMsgs.size(), 1);
    EXPECT_EQ(completedMsgs.size(), 1);
    EXPECT_EQ(protoMsgs.size(), 1);

    Transfer &c = completedMsgs.front();
    // No error received because overwrite is enabled
    EXPECT_EQ(c.error.get(), (void *) NULL);
    EXPECT_EQ(c.fileSize, 10);

    // File metadata should continue empty as dest file report is not created
    EXPECT_EQ(c.fileMetadata, "");
}


TEST_F (UrlCopyFixture, fileOnlineAndNearline) {
    Transfer original;
    original.source = Uri::parse("mock://host/path?size=10");
    original.destination = Uri::parse("mock://host/path?size_pre=10&time=2&user.status=ONLINE_AND_NEARLINE");
    opts.transfers.push_back(original);

    // Ask for destination file report
    opts.dstFileReport = true;

    UrlCopyProcess proc(opts, *this);
    proc.run();

    EXPECT_EQ(startMsgs.size(), 1);
    EXPECT_EQ(completedMsgs.size(), 1);

    Transfer &c = completedMsgs.front();
    EXPECT_NE(c.error.get(), (void *) NULL);
    EXPECT_EQ(c.error->code(), EEXIST);

    EXPECT_NE(c.fileMetadata, "");

    // Parse metadata containing the destination file report
    Json::Value metadata;
    std::istringstream valueStream(c.fileMetadata);
    valueStream >> metadata;

    EXPECT_EQ(metadata.isMember("dst_file"), true);

    Json::Value dst_file = metadata["dst_file"];

    EXPECT_EQ(dst_file["file_on_disk"], true);
    EXPECT_EQ(dst_file["file_on_tape"], true);
}


TEST_F (UrlCopyFixture, FileOnline) {
    Transfer original;
    original.source = Uri::parse("mock://host/path?size=10");
    original.destination = Uri::parse("mock://host/path?size_pre=10&time=2&user.status=ONLINE");
    opts.transfers.push_back(original);

    // Ask for destination file report
    opts.dstFileReport = true;

    UrlCopyProcess proc(opts, *this);
    proc.run();

    EXPECT_EQ(startMsgs.size(), 1);
    EXPECT_EQ(completedMsgs.size(), 1);

    Transfer &c = completedMsgs.front();
    EXPECT_NE(c.error.get(), (void *) NULL);
    EXPECT_EQ(c.error->code(), EEXIST);

    EXPECT_NE(c.fileMetadata, "");

    // Parse metadata containing the destination file report
    Json::Value metadata;
    std::istringstream valueStream(c.fileMetadata);
    valueStream >> metadata;

    EXPECT_EQ(metadata.isMember("dst_file"), true);

    Json::Value dst_file = metadata["dst_file"];

    EXPECT_EQ(dst_file["file_on_disk"], true);
    EXPECT_EQ(dst_file["file_on_tape"], false);
}


TEST_F (UrlCopyFixture, FileNearline) {
    Transfer original;
    original.source = Uri::parse("mock://host/path?size=10");
    original.destination = Uri::parse("mock://host/path?size_pre=10&time=2&user.status=NEARLINE");
    opts.transfers.push_back(original);

    // Ask for destination file report
    opts.dstFileReport = true;

    UrlCopyProcess proc(opts, *this);
    proc.run();

    EXPECT_EQ(startMsgs.size(), 1);
    EXPECT_EQ(completedMsgs.size(), 1);

    Transfer &c = completedMsgs.front();
    EXPECT_NE(c.error.get(), (void *) NULL);
    EXPECT_EQ(c.error->code(), EEXIST);

    EXPECT_NE(c.fileMetadata, "");

    // Parse metadata containing the destination file report
    Json::Value metadata;
    std::istringstream valueStream(c.fileMetadata);
    valueStream >> metadata;

    EXPECT_EQ(metadata.isMember("dst_file"), true);

    Json::Value dst_file = metadata["dst_file"];

    EXPECT_EQ(dst_file["file_on_disk"], false);
    EXPECT_EQ(dst_file["file_on_tape"], true);
}


TEST_F (UrlCopyFixture, InvalidStatus) {
    Transfer original;
    original.source = Uri::parse("mock://host/path?size=10");
    original.destination = Uri::parse("mock://host/path?size_pre=10&time=2&user.status=INVALID");
    opts.transfers.push_back(original);

    // Ask for destination file report
    opts.dstFileReport = true;

    UrlCopyProcess proc(opts, *this);
    proc.run();

    EXPECT_EQ(startMsgs.size(), 1);
    EXPECT_EQ(completedMsgs.size(), 1);

    Transfer &c = completedMsgs.front();
    EXPECT_NE(c.error.get(), (void *) NULL);
    EXPECT_EQ(c.error->code(), EEXIST);

    EXPECT_NE(c.fileMetadata, "");

    // Parse metadata containing the destination file report
    Json::Value metadata;
    std::istringstream valueStream(c.fileMetadata);
    valueStream >> metadata;

    EXPECT_EQ(metadata.isMember("dst_file"), true);

    Json::Value dst_file = metadata["dst_file"];

    EXPECT_EQ(dst_file["file_on_disk"], false);
    EXPECT_EQ(dst_file["file_on_tape"], false);
}


TEST_F (UrlCopyFixture, Md5Checksum) {
    Transfer original;
    original.source = Uri::parse("mock://host/path?size=10");
    original.destination = Uri::parse("mock://host/path?size_pre=10&time=2&user.status=ONLINE&checksum=abc123ab");
    original.checksumAlgorithm = "MD5";
    opts.transfers.push_back(original);

    // Ask for destination file report
    opts.dstFileReport = true;

    UrlCopyProcess proc(opts, *this);
    proc.run();

    EXPECT_EQ(startMsgs.size(), 1);
    EXPECT_EQ(completedMsgs.size(), 1);

    Transfer &c = completedMsgs.front();
    EXPECT_NE(c.error.get(), (void *) NULL);
    EXPECT_EQ(c.error->code(), EEXIST);

    EXPECT_NE(c.fileMetadata, "");

    // Parse metadata containing the destination file report
    Json::Value metadata;
    std::istringstream valueStream(c.fileMetadata);
    valueStream >> metadata;

    EXPECT_EQ(metadata.isMember("dst_file"), true);

    Json::Value dst_file = metadata["dst_file"];

    EXPECT_EQ(dst_file["checksum_type"], "MD5");
    EXPECT_EQ(dst_file["checksum_value"], "abc123ab");
}


TEST_F (UrlCopyFixture, MaxFileSize) {
    Transfer original;
    uint64_t max_file_size = UINT64_MAX;

    std::stringstream source_url;
    source_url << "mock://host/path?size=" << max_file_size;

    std::stringstream dest_url;
    dest_url << "mock://host/path?time=2&user.status=ONLINE&checksum=abc123ab&size_pre=" << max_file_size
             << "&size_post=" << max_file_size;

    original.source = Uri::parse(source_url.str());
    original.destination = Uri::parse(dest_url.str());

    opts.transfers.push_back(original);

    // Ask for destination file report
    opts.dstFileReport = true;

    UrlCopyProcess proc(opts, *this);
    proc.run();

    EXPECT_EQ(startMsgs.size(), 1);
    EXPECT_EQ(completedMsgs.size(), 1);

    Transfer &c = completedMsgs.front();
    EXPECT_NE(c.error.get(), (void *) NULL);
    EXPECT_EQ(c.error->code(), EEXIST);

    EXPECT_NE(c.fileMetadata, "");

    // Parse metadata containing the destination file report
    Json::Value metadata;
    std::istringstream valueStream(c.fileMetadata);
    valueStream >> metadata;

    EXPECT_EQ(metadata.isMember("dst_file"), true);

    Json::Value dst_file = metadata["dst_file"];

    // Make sure that writing/reading from/to the JSON does not corrupt the value of file_size
    EXPECT_EQ(dst_file["file_size"].asUInt64(), max_file_size);
}


TEST_F (UrlCopyFixture, FileMetadataExists) {
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

    EXPECT_EQ(startMsgs.size(), 1);
    EXPECT_EQ(completedMsgs.size(), 1);

    Transfer &c = completedMsgs.front();
    EXPECT_NE(c.error.get(), (void *) NULL);
    EXPECT_EQ(c.error->code(), EEXIST);

    EXPECT_NE(c.fileMetadata, "");

    // Parse metadata containing the destination file report
    Json::Value metadata;
    std::istringstream valueStream(c.fileMetadata);
    valueStream >> metadata;

    EXPECT_EQ(metadata.isMember("dst_file"), true);
    EXPECT_EQ(metadata.isMember("file_metadata"), true);

    EXPECT_EQ(metadata["file_metadata"], test_metadata);
}


TEST_F (UrlCopyFixture, FileMetadataExistsAsJSON) {
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

    EXPECT_EQ(startMsgs.size(), 1);
    EXPECT_EQ(completedMsgs.size(), 1);

    Transfer &c = completedMsgs.front();
    EXPECT_NE(c.error.get(), (void *) NULL);
    EXPECT_EQ(c.error->code(), EEXIST);

    EXPECT_NE(c.fileMetadata, "");

    // Parse metadata containing the destination file report
    Json::Value metadata;
    std::istringstream valueStream(c.fileMetadata);
    valueStream >> metadata;

    EXPECT_EQ(metadata.isMember("dst_file"), true);
    EXPECT_EQ(metadata.isMember("activity"), true);
    EXPECT_EQ(metadata.isMember("priority"), true);

    EXPECT_EQ(metadata["activity"], "Test");
    EXPECT_EQ(metadata["priority"].asUInt(), 5);
}


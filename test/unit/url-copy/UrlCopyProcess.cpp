/*
 * Copyright (c) CERN 2016
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
#include "UrlCopyFixture.h"

BOOST_AUTO_TEST_SUITE(url_copy)

BOOST_FIXTURE_TEST_CASE (simpleTransfer, UrlCopyFixture)
{
    Transfer original;
    original.source      = Uri::parse("mock://host/path/file?size=10");
    original.destination = Uri::parse("mock://host/path/file?size_post=10&time=2");
    opts.transfers.push_back(original);

    UrlCopyProcess proc(opts, *this);
    proc.run();

    BOOST_CHECK_EQUAL(startMsgs.size(), 1);
    BOOST_CHECK_EQUAL(completedMsgs.size(), 1);
    BOOST_CHECK_EQUAL(protoMsgs.size(), 1);

    Transfer &c = completedMsgs.front();
    BOOST_CHECK_EQUAL(c.error.get(), (void*)NULL);
    BOOST_CHECK_EQUAL(c.fileSize, 10);
    BOOST_CHECK_NE(c.stats.process.start, 0);
    BOOST_CHECK_NE(c.stats.process.end, 0);
    BOOST_CHECK_NE(c.stats.transfer.start, 0);
    BOOST_CHECK_NE(c.stats.transfer.end, 0);
}

// User checksum missmatch source checksum
BOOST_FIXTURE_TEST_CASE (sourceUserChecksumMissmatch, UrlCopyFixture)
{
    Transfer original;
    original.source        = Uri::parse("mock://host/path/file?checksum=42");
    original.checksumValue = "24";
    original.checksumAlgorithm = "ADLER32";
    original.destination   = Uri::parse("mock://host/path/file");
    original.checksumMode  = Transfer::Checksum_mode::CHECKSUM_SOURCE;
    opts.transfers.push_back(original);

    UrlCopyProcess proc(opts, *this);
    proc.run();

    BOOST_CHECK_EQUAL(startMsgs.size(), 1);
    BOOST_CHECK_EQUAL(completedMsgs.size(), 1);
    BOOST_CHECK_EQUAL(protoMsgs.size(), 1);

    Transfer &t = completedMsgs.front();
    BOOST_CHECK_NE(t.error.get(), (void*)NULL);
    BOOST_CHECK_EQUAL(t.error->code(), EIO);
    const std::string error_message = "Source and user-defined " + original.checksumAlgorithm +
                                      " checksum do not match (00000042 != " + original.checksumValue +")";
    BOOST_CHECK_EQUAL(t.error->scope(), TRANSFER);
    BOOST_CHECK_EQUAL(t.error->phase(), TRANSFER_PREPARATION);
    BOOST_CHECK_EQUAL(t.error->what(), error_message);
}

// User checksum match source checksum
BOOST_FIXTURE_TEST_CASE (sourceUserChecksumMatch, UrlCopyFixture)
{
    Transfer original;
    original.source        = Uri::parse("mock://host/path/file?checksum=42");
    original.checksumValue = "42";
    original.checksumAlgorithm = "ADLER32";
    original.destination   = Uri::parse("mock://host/path/file?checksum=42");
    original.checksumMode  = Transfer::Checksum_mode::CHECKSUM_SOURCE;
    opts.transfers.push_back(original);

    UrlCopyProcess proc(opts, *this);
    proc.run();

    BOOST_CHECK_EQUAL(startMsgs.size(), 1);
    BOOST_CHECK_EQUAL(completedMsgs.size(), 1);
    BOOST_CHECK_EQUAL(protoMsgs.size(), 1);

    Transfer &t = completedMsgs.front();
    BOOST_CHECK_NE(t.stats.process.start, 0);
    BOOST_CHECK_NE(t.stats.process.end, 0);
    BOOST_CHECK_NE(t.stats.transfer.start, 0);
    BOOST_CHECK_NE(t.stats.transfer.end, 0);
    BOOST_CHECK_EQUAL(t.error.get(), (void*)NULL);
}

// User checksum missmatch destination checksum
BOOST_FIXTURE_TEST_CASE (destinationUserChecksumMissmatch, UrlCopyFixture)
{
    Transfer original;
    original.source        = Uri::parse("mock://host/path/file?checksum=42");
    original.checksumValue = "24";
    original.checksumAlgorithm = "ADLER32";
    original.destination   = Uri::parse("mock://host/path/file?checksum=42");
    original.checksumMode  = Transfer::Checksum_mode::CHECKSUM_TARGET;
    opts.transfers.push_back(original);

    UrlCopyProcess proc(opts, *this);
    proc.run();

    BOOST_CHECK_EQUAL(startMsgs.size(), 1);
    BOOST_CHECK_EQUAL(completedMsgs.size(), 1);
    BOOST_CHECK_EQUAL(protoMsgs.size(), 1);

    Transfer &t = completedMsgs.front();
    BOOST_CHECK_NE(t.error.get(), (void*)NULL);
    BOOST_CHECK_EQUAL(t.error->code(), EIO);
    const std::string error_message = "User-defined and destination " + original.checksumAlgorithm +
        " checksum do not match (" + original.checksumValue + " != 00000042)";
    BOOST_CHECK_EQUAL(t.error->scope(), TRANSFER);
    BOOST_CHECK_EQUAL(t.error->phase(), TRANSFER_FINALIZATION);
    BOOST_CHECK_EQUAL(t.error->what(), error_message);
}

// User checksum match destination checksum
BOOST_FIXTURE_TEST_CASE (destinationUserChecksumMatch, UrlCopyFixture)
{
    Transfer original;
    original.source        = Uri::parse("mock://host/path/file?checksum=42");
    original.checksumValue = "42";
    original.checksumAlgorithm = "ADLER32";
    original.destination   = Uri::parse("mock://host/path/file?checksum=42");
    original.checksumMode  = Transfer::Checksum_mode::CHECKSUM_TARGET;
    opts.transfers.push_back(original);

    UrlCopyProcess proc(opts, *this);
    proc.run();

    BOOST_CHECK_EQUAL(startMsgs.size(), 1);
    BOOST_CHECK_EQUAL(completedMsgs.size(), 1);
    BOOST_CHECK_EQUAL(protoMsgs.size(), 1);

    Transfer &t = completedMsgs.front();
    BOOST_CHECK_NE(t.stats.process.start, 0);
    BOOST_CHECK_NE(t.stats.process.end, 0);
    BOOST_CHECK_NE(t.stats.transfer.start, 0);
    BOOST_CHECK_NE(t.stats.transfer.end, 0);
    BOOST_CHECK_EQUAL(t.error.get(), (void*)NULL);
}

// Source and destination checksum match
BOOST_FIXTURE_TEST_CASE (destinationSourceChecksumMatch, UrlCopyFixture)
{
    Transfer original;
    original.source        = Uri::parse("mock://host/path/file?checksum=42");
    original.destination   = Uri::parse("mock://host/path/file?checksum=42");
    opts.transfers.push_back(original);

    UrlCopyProcess proc(opts, *this);
    proc.run();

    BOOST_CHECK_EQUAL(startMsgs.size(), 1);
    BOOST_CHECK_EQUAL(completedMsgs.size(), 1);
    BOOST_CHECK_EQUAL(protoMsgs.size(), 1);

    Transfer &t = completedMsgs.front();
    BOOST_CHECK_NE(t.stats.process.start, 0);
    BOOST_CHECK_NE(t.stats.process.end, 0);
    BOOST_CHECK_NE(t.stats.transfer.start, 0);
    BOOST_CHECK_NE(t.stats.transfer.end, 0);
    BOOST_CHECK_EQUAL(t.error.get(), (void*)NULL);
}

// Source and destination checksum missmatch
BOOST_FIXTURE_TEST_CASE (destinationSourceChecksumMissMatch, UrlCopyFixture)
{
    Transfer original;
    original.source        = Uri::parse("mock://host/path/file?checksum=24");
    original.destination   = Uri::parse("mock://host/path/file?checksum=42");
    opts.transfers.push_back(original);

    UrlCopyProcess proc(opts, *this);
    proc.run();

    BOOST_CHECK_EQUAL(startMsgs.size(), 1);
    BOOST_CHECK_EQUAL(completedMsgs.size(), 1);
    BOOST_CHECK_EQUAL(protoMsgs.size(), 1);

    Transfer &t = completedMsgs.front();
    BOOST_CHECK_NE(t.error.get(), (void*)NULL);
    BOOST_CHECK_EQUAL(t.error->code(), EIO);
    const std::string error_message = "Source and destination " + original.checksumAlgorithm +
        " checksum do not match (00000024 != 00000042)";
    BOOST_CHECK_EQUAL(t.error->scope(), TRANSFER);
    BOOST_CHECK_EQUAL(t.error->phase(), TRANSFER_FINALIZATION);
    BOOST_CHECK_EQUAL(t.error->what(), error_message);
}

// Destination exist
BOOST_FIXTURE_TEST_CASE (destinationExist, UrlCopyFixture)
{
    Transfer original;
    original.source      = Uri::parse("mock://host/file");
    original.destination = Uri::parse("mock://host/file?exists=1");
    opts.transfers.push_back(original);

    UrlCopyProcess proc(opts, *this);
    proc.run();

    BOOST_CHECK_EQUAL(startMsgs.size(), 1);
    BOOST_CHECK_EQUAL(completedMsgs.size(), 1);
    BOOST_CHECK_EQUAL(protoMsgs.size(), 1);

    Transfer &t = completedMsgs.front();
    BOOST_CHECK_NE(t.error.get(), (void*)NULL);
    BOOST_CHECK_EQUAL(t.error->code(), EEXIST);
    const std::string &error_message = "Destination file exists and overwrite is not enabled";
    BOOST_CHECK_EQUAL(t.error->scope(), TRANSFER);
    BOOST_CHECK_EQUAL(t.error->phase(), TRANSFER_PREPARATION);
    BOOST_CHECK_EQUAL(t.error->what(), error_message);
}

// Destination doesn't exist
BOOST_FIXTURE_TEST_CASE (destinationDoesNotExist, UrlCopyFixture)
{
    Transfer original;

    original.source      = Uri::parse("mock://host/file");
    original.destination = Uri::parse("mock://host/file?exists=0");
    opts.transfers.push_back(original);

    UrlCopyProcess proc(opts, *this);
    proc.run();

    BOOST_CHECK_EQUAL(startMsgs.size(), 1);
    BOOST_CHECK_EQUAL(completedMsgs.size(), 1);
    BOOST_CHECK_EQUAL(protoMsgs.size(), 1);

    Transfer &t = completedMsgs.front();
    BOOST_CHECK_EQUAL(t.error.get(), (void*)NULL);
    BOOST_CHECK_NE(t.stats.process.start, 0);
    BOOST_CHECK_NE(t.stats.process.end, 0);
    BOOST_CHECK_NE(t.stats.transfer.start, 0);
    BOOST_CHECK_NE(t.stats.transfer.end, 0);
}

// Destination doesn't exist, fail the transfer for write permission: EPERM
BOOST_FIXTURE_TEST_CASE (destinationDoesNotExistNoRW, UrlCopyFixture)
{
    Transfer original;
    original.source      = Uri::parse("mock://host/path/file");
    original.destination = Uri::parse("mock://host/path/file?time=2&transfer_errno=1");
    opts.transfers.push_back(original);

    UrlCopyProcess proc(opts, *this);
    proc.run();

    BOOST_CHECK_EQUAL(startMsgs.size(), 1);
    BOOST_CHECK_EQUAL(completedMsgs.size(), 1);
    BOOST_CHECK_EQUAL(protoMsgs.size(), 1);

    Transfer &t = completedMsgs.front();
    BOOST_CHECK_NE(t.error.get(), (void*)NULL);
    BOOST_CHECK_EQUAL(t.error->code(), EPERM);
    BOOST_CHECK_EQUAL(t.error->scope(), TRANSFER);
    BOOST_CHECK_EQUAL(t.error->phase(), TRANSFER);
}


// Destination parent path does not exist, cannot create parent
BOOST_FIXTURE_TEST_CASE (destinationNoParentPath, UrlCopyFixture)
{
    Transfer original;

    original.source          = Uri::parse("mock://host/file");
    std::string rd_only_path = "&rd_path=mock://host/super/path/";
    original.destination     = Uri::parse("mock://host/super/path/file?exists=0" + rd_only_path);
    opts.transfers.push_back(original);

    UrlCopyProcess proc(opts, *this);
    proc.run();

    BOOST_CHECK_EQUAL(startMsgs.size(), 1);
    BOOST_CHECK_EQUAL(completedMsgs.size(), 1);
    BOOST_CHECK_EQUAL(protoMsgs.size(), 1);

    Transfer &t = completedMsgs.front();
    BOOST_CHECK_NE(t.error.get(), (void*)NULL);
    BOOST_CHECK_EQUAL(t.error->code(), EPERM);
    BOOST_CHECK_EQUAL(t.error->scope(), TRANSFER);
    BOOST_CHECK_EQUAL(t.error->phase(), TRANSFER_PREPARATION);
}

BOOST_FIXTURE_TEST_CASE (panic, UrlCopyFixture)
{
    Transfer original;
    original.source      = Uri::parse("mock://host/path/file?size=10");
    original.destination = Uri::parse("mock://host/path/file?size_post=10&time=2");
    opts.transfers.push_back(original);

    UrlCopyProcess proc(opts, *this);
    proc.panic("TEST PANIC MESSAGE");

    BOOST_CHECK_EQUAL(completedMsgs.size(), 1);

    Transfer &t = completedMsgs.front();

    BOOST_CHECK_NE(t.error.get(), (void*)NULL);
    BOOST_CHECK_EQUAL(original.source.fullUri, t.source.fullUri);
    BOOST_CHECK_EQUAL(original.destination.fullUri, t.destination.fullUri);
    BOOST_CHECK_EQUAL(t.error->code(), EINTR);
    BOOST_CHECK_EQUAL(t.error->scope(), "GENERAL_FAILURE");
    BOOST_CHECK_EQUAL(t.error->what(), "TEST PANIC MESSAGE");
}


BOOST_FIXTURE_TEST_CASE (cancel, UrlCopyFixture)
{
    Transfer original;
    const std::string &rw_path = "&rw_path=mock://host/path/";
    original.source      = Uri::parse("mock://host/path/file?size=10");
    original.destination = Uri::parse("mock://host/path/file?size_post=10&time=5" + rw_path);
    opts.transfers.push_back(original);

    UrlCopyProcess proc(opts, *this);
    boost::thread thread(boost::bind(&UrlCopyProcess::run, &proc));
    boost::this_thread::sleep(boost::posix_time::seconds(1));
    proc.cancel();
    thread.join();

    BOOST_CHECK_EQUAL(startMsgs.size(), 1);
    BOOST_CHECK_EQUAL(completedMsgs.size(), 1);

    Transfer &c = completedMsgs.front();
    BOOST_CHECK_NE(c.error.get(), (void*)NULL);
    BOOST_CHECK_EQUAL(c.error->code(), ECANCELED);
    BOOST_CHECK_EQUAL(c.fileSize, 10);
}


BOOST_FIXTURE_TEST_CASE (timeout, UrlCopyFixture)
{
    Transfer original;
    original.source      = Uri::parse("mock://host/path/file?size=10");
    original.destination = Uri::parse("mock://host/path/file?size_post=10&time=100");
    opts.transfers.push_back(original);
    opts.timeout = 1; // Real timeout = 60 + opts.timeout

    UrlCopyProcess proc(opts, *this);
    boost::thread thread(boost::bind(&UrlCopyProcess::run, &proc));
    boost::this_thread::sleep(boost::posix_time::seconds(65));
    thread.join();

    BOOST_CHECK_EQUAL(startMsgs.size(), 1);
    BOOST_CHECK_EQUAL(completedMsgs.size(), 1);

    Transfer &c = completedMsgs.front();
    BOOST_CHECK_NE(c.error.get(), (void*)NULL);
    BOOST_CHECK_EQUAL(c.error->code(), ETIMEDOUT);
    BOOST_CHECK_EQUAL(c.fileSize, 10);
}


BOOST_FIXTURE_TEST_CASE (multipleSimple, UrlCopyFixture)
{
    Transfer original, original2;
    original.source       = Uri::parse("mock://host/path/file?size=10");
    original.destination  = Uri::parse("mock://host/path/file?size_post=10&time=2");
    original2.source      = Uri::parse("mock://host/path/file2?size=42");
    original2.destination = Uri::parse("mock://host/path/file2?size_post=42&time=1");
    opts.transfers.push_back(original);
    opts.transfers.push_back(original2);

    UrlCopyProcess proc(opts, *this);
    proc.run();

    BOOST_CHECK_EQUAL(startMsgs.size(), 2);
    BOOST_CHECK_EQUAL(completedMsgs.size(), 2);

    Transfer c = completedMsgs.front();
    BOOST_CHECK_EQUAL(c.error.get(), (void*)NULL);
    BOOST_CHECK_EQUAL(c.fileSize, 10);
    BOOST_CHECK_NE(c.stats.process.start, 0);
    BOOST_CHECK_NE(c.stats.process.end, 0);
    BOOST_CHECK_NE(c.stats.transfer.start, 0);
    BOOST_CHECK_NE(c.stats.transfer.end, 0);

    completedMsgs.pop_front();
    c = completedMsgs.front();
    BOOST_CHECK_EQUAL(c.error.get(), (void*)NULL);
    BOOST_CHECK_EQUAL(c.fileSize, 42);
    BOOST_CHECK_NE(c.stats.process.start, 0);
    BOOST_CHECK_NE(c.stats.process.end, 0);
    BOOST_CHECK_NE(c.stats.transfer.start, 0);
    BOOST_CHECK_NE(c.stats.transfer.end, 0);
}


BOOST_FIXTURE_TEST_CASE (multipleCancel, UrlCopyFixture)
{
    Transfer original, original2;
    original.source       = Uri::parse("mock://host/path?size=10");
    original.destination  = Uri::parse("mock://host/path?size_post=10&time=2");
    original2.source      = Uri::parse("mock://host/path2?size=42");
    original2.destination = Uri::parse("mock://host/path2?size_post=42&time=10");
    opts.transfers.push_back(original);
    opts.transfers.push_back(original2);

    UrlCopyProcess proc(opts, *this);
    boost::thread thread(boost::bind(&UrlCopyProcess::run, &proc));
    boost::this_thread::sleep(boost::posix_time::seconds(4));
    proc.cancel();
    thread.join();

    BOOST_CHECK_EQUAL(startMsgs.size(), 2);
    BOOST_CHECK_EQUAL(completedMsgs.size(), 2);

    Transfer c = completedMsgs.front();
    BOOST_CHECK_EQUAL(c.error.get(), (void*)NULL);
    BOOST_CHECK_EQUAL(c.fileSize, 10);

    completedMsgs.pop_front();
    c = completedMsgs.front();
    BOOST_CHECK_NE(c.error.get(), (void*)NULL);
    BOOST_CHECK_EQUAL(c.error->code(), ECANCELED);
    BOOST_CHECK_EQUAL(c.fileSize, 42);
}


BOOST_FIXTURE_TEST_CASE (multiplePanic, UrlCopyFixture)
{
    Transfer original, original2;
    original.source       = Uri::parse("mock://host/path?size=10");
    original.destination  = Uri::parse("mock://host/path?size_post=10&time=2");
    original2.source      = Uri::parse("mock://host/path2?size=42");
    original2.destination = Uri::parse("mock://host/path2?size_post=42&time=10");
    opts.transfers.push_back(original);
    opts.transfers.push_back(original2);

    UrlCopyProcess proc(opts, *this);
    boost::thread thread(boost::bind(&UrlCopyProcess::run, &proc));
    boost::this_thread::sleep(boost::posix_time::seconds(4));
    proc.panic("Signal 385");

    BOOST_CHECK_EQUAL(startMsgs.size(), 2);
    BOOST_CHECK_EQUAL(completedMsgs.size(), 2);

    Transfer c = completedMsgs.front();
    BOOST_CHECK_EQUAL(c.error.get(), (void*)NULL);
    BOOST_CHECK_EQUAL(c.fileSize, 10);

    completedMsgs.pop_front();
    c = completedMsgs.front();
    BOOST_CHECK_NE(c.error.get(), (void*)NULL);
    BOOST_CHECK_EQUAL(c.error->code(), EINTR);
    BOOST_CHECK_EQUAL(c.error->what(), "Signal 385");

    thread.join();
}


BOOST_AUTO_TEST_SUITE_END()

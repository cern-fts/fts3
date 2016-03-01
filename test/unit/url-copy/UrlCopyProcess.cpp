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

#include <boost/test/included/unit_test.hpp>
#include "url-copy/UrlCopyProcess.h"


BOOST_AUTO_TEST_SUITE(url_copy)


class UrlCopyFixture: public Reporter {
protected:
    UrlCopyOpts opts;
    std::list<Transfer> completedMsgs, startMsgs, pingMsgs, protoMsgs;

public:
    UrlCopyFixture() {
        opts.logToStderr = false;
        opts.logDir = "/tmp/fts3-tests";
    }

    void sendTransferStart(const Transfer &t, Gfal2TransferParams&) {
        startMsgs.push_back(t);
    }

    void sendProtocol(const Transfer &t, Gfal2TransferParams&) {
        protoMsgs.push_back(t);
    }

    void sendTransferCompleted(const Transfer &t, Gfal2TransferParams&) {
        completedMsgs.push_back(t);
    }

    void sendPing(const Transfer &t) {
        pingMsgs.push_back(t);
    }
};


BOOST_FIXTURE_TEST_CASE (simpleTransfer, UrlCopyFixture)
{
    Transfer original;
    original.source = Uri::parse("mock://host/path?size=10");
    original.destination = Uri::parse("mock://host/path?size_post=10&transfer_time=2s");
    opts.transfers.push_back(original);

    UrlCopyProcess proc(opts, *this);
    proc.run();

    BOOST_CHECK_EQUAL(startMsgs.size(), 1);
    BOOST_CHECK_EQUAL(completedMsgs.size(), 1);
    BOOST_CHECK_EQUAL(protoMsgs.size(), 1);

    Transfer &c = completedMsgs.front();
    BOOST_CHECK(!(static_cast<bool>(c.error)));
    BOOST_CHECK_EQUAL(c.fileSize, 10);
    BOOST_CHECK_NE(c.stats.process.start, 0);
    BOOST_CHECK_NE(c.stats.process.end, 0);
    BOOST_CHECK_NE(c.stats.transfer.start, 0);
    BOOST_CHECK_NE(c.stats.transfer.end, 0);
}


BOOST_FIXTURE_TEST_CASE (panic, UrlCopyFixture)
{
    Transfer original;
    original.source = Uri::parse("mock://host/path?size=10");
    original.destination = Uri::parse("mock://host/path?size_post=10&transfer_time=2s");
    opts.transfers.push_back(original);

    UrlCopyProcess proc(opts, *this);
    proc.panic("TEST PANIC MESSAGE");

    BOOST_CHECK_EQUAL(completedMsgs.size(), 1);

    Transfer &t = completedMsgs.front();

    BOOST_CHECK(static_cast<bool>(t.error));
    BOOST_CHECK_EQUAL(original.source.fullUri, t.source.fullUri);
    BOOST_CHECK_EQUAL(original.destination.fullUri, t.destination.fullUri);
    BOOST_CHECK_EQUAL(t.error->code(), EINTR);
    BOOST_CHECK_EQUAL(t.error->scope(), "GENERAL_FAILURE");
    BOOST_CHECK_EQUAL(t.error->what(), "TEST PANIC MESSAGE");
}


BOOST_FIXTURE_TEST_CASE (cancel, UrlCopyFixture)
{
    Transfer original;
    original.source = Uri::parse("mock://host/path?size=10");
    original.destination = Uri::parse("mock://host/path?size_post=10&transfer_time=5s");
    opts.transfers.push_back(original);

    UrlCopyProcess proc(opts, *this);
    boost::thread thread(boost::bind(&UrlCopyProcess::run, &proc));
    boost::this_thread::sleep(boost::posix_time::seconds(1));
    proc.cancel();
    thread.join();

    BOOST_CHECK_EQUAL(startMsgs.size(), 1);
    BOOST_CHECK_EQUAL(completedMsgs.size(), 1);

    Transfer &c = completedMsgs.front();
    BOOST_CHECK(static_cast<bool>(c.error));
    BOOST_CHECK_EQUAL(c.error->code(), ECANCELED);
    BOOST_CHECK_EQUAL(c.fileSize, 10);
}


BOOST_FIXTURE_TEST_CASE (timeout, UrlCopyFixture)
{
    Transfer original;
    original.source = Uri::parse("mock://host/path?size=10");
    original.destination = Uri::parse("mock://host/path?size_post=10&transfer_time=10s");
    opts.transfers.push_back(original);
    opts.timeout = 1;

    UrlCopyProcess proc(opts, *this);
    boost::thread thread(boost::bind(&UrlCopyProcess::run, &proc));
    boost::this_thread::sleep(boost::posix_time::seconds(1));
    thread.join();

    BOOST_CHECK_EQUAL(startMsgs.size(), 1);
    BOOST_CHECK_EQUAL(completedMsgs.size(), 1);

    Transfer &c = completedMsgs.front();
    BOOST_CHECK(static_cast<bool>(c.error));
    BOOST_CHECK_EQUAL(c.error->code(), ETIMEDOUT);
    BOOST_CHECK_EQUAL(c.fileSize, 10);
}


BOOST_FIXTURE_TEST_CASE (multipleSimple, UrlCopyFixture)
{
    Transfer original, original2;
    original.source = Uri::parse("mock://host/path?size=10");
    original.destination = Uri::parse("mock://host/path?size_post=10&transfer_time=2s");
    original2.source = Uri::parse("mock://host/path2?size=42");
    original2.destination = Uri::parse("mock://host/path2?size_post=42&transfer_time=1s");
    opts.transfers.push_back(original);
    opts.transfers.push_back(original2);

    UrlCopyProcess proc(opts, *this);
    proc.run();

    BOOST_CHECK_EQUAL(startMsgs.size(), 2);
    BOOST_CHECK_EQUAL(completedMsgs.size(), 2);

    Transfer c = completedMsgs.front();
    BOOST_CHECK(!(static_cast<bool>(c.error)));
    BOOST_CHECK_EQUAL(c.fileSize, 10);
    BOOST_CHECK_NE(c.stats.process.start, 0);
    BOOST_CHECK_NE(c.stats.process.end, 0);
    BOOST_CHECK_NE(c.stats.transfer.start, 0);
    BOOST_CHECK_NE(c.stats.transfer.end, 0);

    completedMsgs.pop_front();
    c = completedMsgs.front();
    BOOST_CHECK(!(static_cast<bool>(c.error)));
    BOOST_CHECK_EQUAL(c.fileSize, 42);
    BOOST_CHECK_NE(c.stats.process.start, 0);
    BOOST_CHECK_NE(c.stats.process.end, 0);
    BOOST_CHECK_NE(c.stats.transfer.start, 0);
    BOOST_CHECK_NE(c.stats.transfer.end, 0);
}


BOOST_FIXTURE_TEST_CASE (multipleCancel, UrlCopyFixture)
{
    Transfer original, original2;
    original.source = Uri::parse("mock://host/path?size=10");
    original.destination = Uri::parse("mock://host/path?size_post=10&transfer_time=2s");
    original2.source = Uri::parse("mock://host/path2?size=42");
    original2.destination = Uri::parse("mock://host/path2?size_post=42&transfer_time=10s");
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
    BOOST_CHECK(!(static_cast<bool>(c.error)));
    BOOST_CHECK_EQUAL(c.fileSize, 10);

    completedMsgs.pop_front();
    c = completedMsgs.front();
    BOOST_CHECK(static_cast<bool>(c.error));
    BOOST_CHECK_EQUAL(c.error->code(), ECANCELED);
    BOOST_CHECK_EQUAL(c.fileSize, 42);
}


BOOST_FIXTURE_TEST_CASE (multiplePanic, UrlCopyFixture)
{
    Transfer original, original2;
    original.source = Uri::parse("mock://host/path?size=10");
    original.destination = Uri::parse("mock://host/path?size_post=10&transfer_time=2s");
    original2.source = Uri::parse("mock://host/path2?size=42");
    original2.destination = Uri::parse("mock://host/path2?size_post=42&transfer_time=10s");
    opts.transfers.push_back(original);
    opts.transfers.push_back(original2);

    UrlCopyProcess proc(opts, *this);
    boost::thread thread(boost::bind(&UrlCopyProcess::run, &proc));
    boost::this_thread::sleep(boost::posix_time::seconds(4));
    proc.panic("Signal 385");

    BOOST_CHECK_EQUAL(startMsgs.size(), 2);
    BOOST_CHECK_EQUAL(completedMsgs.size(), 2);

    Transfer c = completedMsgs.front();
    BOOST_CHECK(!(static_cast<bool>(c.error)));
    BOOST_CHECK_EQUAL(c.fileSize, 10);

    completedMsgs.pop_front();
    c = completedMsgs.front();
    BOOST_CHECK(static_cast<bool>(c.error));
    BOOST_CHECK_EQUAL(c.error->code(), EINTR);
    BOOST_CHECK_EQUAL(c.error->what(), "Signal 385");

    thread.join();
}


BOOST_FIXTURE_TEST_CASE (multihop, UrlCopyFixture)
{
    Transfer original, original2;
    original.source = Uri::parse("mock://host/path?size=10&errno=2");
    original.destination = Uri::parse("mock://host/path?size_post=10&transfer_time=2s");
    original.fileId = 10;
    original2.source = Uri::parse("mock://host/path2?size=42");
    original2.destination = Uri::parse("mock://host/path2?size_post=42&transfer_time=2s");
    original2.fileId = 11;
    opts.transfers.push_back(original);
    opts.transfers.push_back(original2);

    UrlCopyProcess proc(opts, *this);
    proc.run();

    BOOST_CHECK_EQUAL(startMsgs.size(), 2);
    BOOST_CHECK_EQUAL(completedMsgs.size(), 2);

    Transfer c = completedMsgs.front();
    BOOST_CHECK((static_cast<bool>(c.error)));
    BOOST_CHECK_EQUAL(c.error->code(), ENOENT);
    BOOST_CHECK_EQUAL(c.fileId, 10);

    completedMsgs.pop_front();
    c = completedMsgs.front();
    BOOST_CHECK(!static_cast<bool>(c.error));
    BOOST_CHECK_EQUAL(c.fileId, 11);
}


BOOST_AUTO_TEST_SUITE_END()

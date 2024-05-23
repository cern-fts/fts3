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

#include "UrlCopyFixture.h"

TEST_F(UrlCopyFixture, SimpleTransfer) {
    Transfer original;
    original.source = Uri::parse("mock://host/path?size=10");
    original.destination = Uri::parse("mock://host/path?size_post=10&time=2");
    opts.transfers.push_back(original);

    UrlCopyProcess proc(opts, *this);
    proc.run();

    EXPECT_EQ(startMsgs.size(), 1);
    EXPECT_EQ(completedMsgs.size(), 1);
    EXPECT_EQ(protoMsgs.size(), 1);

    Transfer &c = completedMsgs.front();
    EXPECT_EQ(c.error.get(), (void *) NULL);
    EXPECT_EQ(c.fileSize, 10);
    EXPECT_NE(c.stats.process.start, 0);
    EXPECT_NE(c.stats.process.end, 0);
    EXPECT_NE(c.stats.transfer.start, 0);
    EXPECT_NE(c.stats.transfer.end, 0);
}


TEST_F(UrlCopyFixture, Panic) {
    Transfer original;
    original.source = Uri::parse("mock://host/path?size=10");
    original.destination = Uri::parse("mock://host/path?size_post=10&time=2");
    opts.transfers.push_back(original);

    UrlCopyProcess proc(opts, *this);
    proc.panic("TEST PANIC MESSAGE");

    EXPECT_EQ(completedMsgs.size(), 1);

    Transfer &t = completedMsgs.front();

    EXPECT_NE(t.error.get(), (void *) NULL);
    EXPECT_EQ(original.source.fullUri, t.source.fullUri);
    EXPECT_EQ(original.destination.fullUri, t.destination.fullUri);
    EXPECT_EQ(t.error->code(), EINTR);
    EXPECT_STREQ(t.error->scope(), "GENERAL_FAILURE");
    EXPECT_STREQ(t.error->what(), "TEST PANIC MESSAGE");
}


TEST_F(UrlCopyFixture, Cancel) {
    Transfer original;
    original.source = Uri::parse("mock://host/path?size=10");
    original.destination = Uri::parse("mock://host/path?size_post=10&time=5");
    opts.transfers.push_back(original);

    UrlCopyProcess proc(opts, *this);
    boost::thread thread(boost::bind(&UrlCopyProcess::run, &proc));
    boost::this_thread::sleep(boost::posix_time::seconds(1));
    proc.cancel();
    thread.join();

    EXPECT_EQ(startMsgs.size(), 1);
    EXPECT_EQ(completedMsgs.size(), 1);

    Transfer &c = completedMsgs.front();
    EXPECT_NE(c.error.get(), (void *) NULL);
    EXPECT_EQ(c.error->code(), ECANCELED);
    EXPECT_EQ(c.fileSize, 10);
}


TEST_F(UrlCopyFixture, Timeout) {
    Transfer original;
    original.source = Uri::parse("mock://host/path?size=10");
    original.destination = Uri::parse("mock://host/path?size_post=10&time=100");
    opts.transfers.push_back(original);
    opts.timeout = 1;

    UrlCopyProcess proc(opts, *this);
    boost::thread thread(boost::bind(&UrlCopyProcess::run, &proc));
    boost::this_thread::sleep(boost::posix_time::seconds(65));
    thread.join();

    EXPECT_EQ(startMsgs.size(), 1);
    EXPECT_EQ(completedMsgs.size(), 1);

    Transfer &c = completedMsgs.front();
    EXPECT_NE(c.error.get(), (void *) NULL);
    EXPECT_EQ(c.error->code(), ETIMEDOUT);
    EXPECT_EQ(c.fileSize, 10);
}


TEST_F(UrlCopyFixture, MultipleSimple) {
    Transfer original, original2;
    original.source = Uri::parse("mock://host/path?size=10");
    original.destination = Uri::parse("mock://host/path?size_post=10&time=2");
    original2.source = Uri::parse("mock://host/path2?size=42");
    original2.destination = Uri::parse("mock://host/path2?size_post=42&time=1");
    opts.transfers.push_back(original);
    opts.transfers.push_back(original2);

    UrlCopyProcess proc(opts, *this);
    proc.run();

    EXPECT_EQ(startMsgs.size(), 2);
    EXPECT_EQ(completedMsgs.size(), 2);

    Transfer c = completedMsgs.front();
    EXPECT_EQ(c.error.get(), (void *) NULL);
    EXPECT_EQ(c.fileSize, 10);
    EXPECT_NE(c.stats.process.start, 0);
    EXPECT_NE(c.stats.process.end, 0);
    EXPECT_NE(c.stats.transfer.start, 0);
    EXPECT_NE(c.stats.transfer.end, 0);

    completedMsgs.pop_front();
    c = completedMsgs.front();
    EXPECT_EQ(c.error.get(), (void *) NULL);
    EXPECT_EQ(c.fileSize, 42);
    EXPECT_NE(c.stats.process.start, 0);
    EXPECT_NE(c.stats.process.end, 0);
    EXPECT_NE(c.stats.transfer.start, 0);
    EXPECT_NE(c.stats.transfer.end, 0);
}


TEST_F(UrlCopyFixture, MultipleCancel) {
    Transfer original, original2;
    original.source = Uri::parse("mock://host/path?size=10");
    original.destination = Uri::parse("mock://host/path?size_post=10&time=2");
    original2.source = Uri::parse("mock://host/path2?size=42");
    original2.destination = Uri::parse("mock://host/path2?size_post=42&time=10");
    opts.transfers.push_back(original);
    opts.transfers.push_back(original2);

    UrlCopyProcess proc(opts, *this);
    boost::thread thread(boost::bind(&UrlCopyProcess::run, &proc));
    boost::this_thread::sleep(boost::posix_time::seconds(4));
    proc.cancel();
    thread.join();

    EXPECT_EQ(startMsgs.size(), 2);
    EXPECT_EQ(completedMsgs.size(), 2);

    Transfer c = completedMsgs.front();
    EXPECT_EQ(c.error.get(), (void *) NULL);
    EXPECT_EQ(c.fileSize, 10);

    completedMsgs.pop_front();
    c = completedMsgs.front();
    EXPECT_NE(c.error.get(), (void *) NULL);
    EXPECT_EQ(c.error->code(), ECANCELED);
    EXPECT_EQ(c.fileSize, 42);
}


TEST_F(UrlCopyFixture, MultiplePanic) {
    Transfer original, original2;
    original.source = Uri::parse("mock://host/path?size=10");
    original.destination = Uri::parse("mock://host/path?size_post=10&time=2");
    original2.source = Uri::parse("mock://host/path2?size=42");
    original2.destination = Uri::parse("mock://host/path2?size_post=42&time=10");
    opts.transfers.push_back(original);
    opts.transfers.push_back(original2);

    UrlCopyProcess proc(opts, *this);
    boost::thread thread(boost::bind(&UrlCopyProcess::run, &proc));
    boost::this_thread::sleep(boost::posix_time::seconds(4));
    proc.panic("Signal 385");

    EXPECT_EQ(startMsgs.size(), 2);
    EXPECT_EQ(completedMsgs.size(), 2);

    Transfer c = completedMsgs.front();
    EXPECT_EQ(c.error.get(), (void *) NULL);
    EXPECT_EQ(c.fileSize, 10);

    completedMsgs.pop_front();
    c = completedMsgs.front();
    EXPECT_NE(c.error.get(), (void *) NULL);
    EXPECT_EQ(c.error->code(), EINTR);
    EXPECT_STREQ(c.error->what(), "Signal 385");

    thread.join();
}

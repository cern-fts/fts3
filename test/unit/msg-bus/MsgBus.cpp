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
#include <boost/filesystem.hpp>
#include <boost/function.hpp>

#include <glib.h>

#include "msg-bus/consumer.h"
#include "msg-bus/producer.h"


bool operator == (const Message &a, const Message &b)
{
    return g_strcmp0(a.job_id, b.job_id) == 0 &&
           g_strcmp0(a.transfer_status, b.transfer_status) == 0 &&
           g_strcmp0(a.transfer_message, b.transfer_message) == 0 &&
           g_strcmp0(a.source_se, b.source_se) == 0 &&
           g_strcmp0(a.dest_se, b.dest_se) == 0 &&
           a.file_id == b.file_id &&
           a.process_id == b.process_id &&
           a.timeInSecs == b.timeInSecs &&
           a.filesize == b.filesize &&
           a.nostreams == b.nostreams &&
           a.timeout == b.timeout &&
           a.buffersize == b.buffersize &&
           a.timestamp == b.timestamp &&
           a.retry == b.retry &&
           a.throughput == b.throughput &&
           a.msg_errno == b.msg_errno;
}


bool operator == (const MessageUpdater &a, const MessageUpdater &b)
{
    return g_strcmp0(a.job_id, b.job_id) == 0 &&
           a.file_id == b.file_id &&
           a.process_id == b.process_id &&
           a.timestamp == b.timestamp &&
           a.throughput == b.throughput &&
           a.transferred == b.transferred &&
           g_strcmp0(a.source_surl, b.source_surl) == 0 &&
           g_strcmp0(a.dest_surl, b.dest_surl) == 0 &&
           g_strcmp0(a.source_turl, b.source_turl) == 0 &&
           g_strcmp0(a.transfer_status, b.transfer_status) == 0 &&
           a.msg_errno == b.msg_errno;
}

bool operator == (const MessageLog &a, const MessageLog &b)
{
    return g_strcmp0(a.job_id, b.job_id) == 0 &&
           a.file_id == b.file_id &&
           g_strcmp0(a.host, b.host) == 0 &&
           g_strcmp0(a.filePath, b.filePath) == 0 &&
           a.debugFile == b.debugFile &&
           a.timestamp == b.timestamp &&
           a.msg_errno == b.msg_errno;
}


bool operator == (const MessageBringonline &a, const MessageBringonline &b)
{
    return a.file_id == b.file_id &&
           g_strcmp0(a.job_id, b.job_id) == 0 &&
           g_strcmp0(a.transfer_status, b.transfer_status) == 0 &&
           g_strcmp0(a.transfer_message, b.transfer_message) == 0 &&
           a.msg_errno == b.msg_errno;
}


bool operator == (const MessageMonitoring &a, const MessageMonitoring &b)
{
    return g_strcmp0(a.msg, b.msg) == 0 &&
           a.timestamp == b.timestamp &&
           a.msg_errno == b.msg_errno;
}


std::ostream& operator << (std::ostream &os, const MessageBase&)
{
    // Pass, needed to compile tests, as BOOST_CHECK_EQUAL will use it
    return os;
}


BOOST_AUTO_TEST_SUITE(MsgBusTest)


class MsgBusFixture {
protected:
    static const std::string TEST_PATH;

public:
    MsgBusFixture() {
        boost::filesystem::create_directories(TEST_PATH);
    }

    ~MsgBusFixture() {
        boost::filesystem::remove_all(TEST_PATH);
    }
};

const std::string MsgBusFixture::TEST_PATH("/tmp/MsgBusTest");


template <typename MSG_CONTAINER>
static void expectZeroMessages(boost::function<int (Consumer*, MSG_CONTAINER&)> func, Consumer &consumer)
{
    MSG_CONTAINER container;
    BOOST_CHECK_EQUAL(0, func(&consumer, container));
    BOOST_CHECK_EQUAL(0, container.size());
}


BOOST_FIXTURE_TEST_CASE (simpleStatus, MsgBusFixture)
{
    Producer producer(TEST_PATH);
    Consumer consumer(TEST_PATH);

    Message original;
    g_strlcpy(original.job_id, "1906cc40-b915-11e5-9a03-02163e006dd0", sizeof(original.job_id));
    g_strlcpy(original.transfer_status, "FAILED", sizeof(original.transfer_status));
    g_strlcpy(original.transfer_message, "TEST FAILURE, EVERYTHING IS TERRIBLE", sizeof(original.transfer_message));
    g_strlcpy(original.source_se, "mock://source/file", sizeof(original.source_se));
    g_strlcpy(original.dest_se, "mock://source/file2", sizeof(original.dest_se));
    original.file_id = 42;
    original.process_id = 1234;
    original.timeInSecs = 55;
    original.filesize = 1023;
    original.nostreams = 33;
    original.timeout = 22;
    original.buffersize = 1;
    original.retry = 5;
    original.retry = 0.2;
    original.msg_errno = EMFILE;

    BOOST_CHECK_EQUAL(0, producer.runProducerStatus(original));

    // Make sure the messages don't get messed up
    expectZeroMessages<std::vector<MessageBringonline>>(&Consumer::runConsumerDeletions, consumer);
    expectZeroMessages<std::vector<MessageBringonline>>(&Consumer::runConsumerStaging, consumer);
    expectZeroMessages<std::map<int, struct MessageLog>>(&Consumer::runConsumerLog, consumer);
    expectZeroMessages<std::vector<MessageMonitoring>>(&Consumer::runConsumerMonitoring, consumer);
    expectZeroMessages<std::vector<MessageUpdater>>(&Consumer::runConsumerStall, consumer);

    // First attempt must return the single message
    std::vector<Message> statuses;
    BOOST_CHECK_EQUAL(0, consumer.runConsumerStatus(statuses));
    BOOST_CHECK_EQUAL(1, statuses.size());
    BOOST_CHECK_EQUAL(statuses[0], original);

    // Second attempt must return empty (already consumed)
    statuses.clear();
    BOOST_CHECK_EQUAL(0, consumer.runConsumerStatus(statuses));
    BOOST_CHECK_EQUAL(0, statuses.size());
}


BOOST_FIXTURE_TEST_CASE (simpleMonitoring, MsgBusFixture)
{
    Producer producer(TEST_PATH);
    Consumer consumer(TEST_PATH);

    MessageMonitoring original;
    g_strlcpy(original.msg, "blah bleh blih bloh cluh", sizeof(original.msg));
    original.msg_errno = 0;

    BOOST_CHECK_EQUAL(0, producer.runProducerMonitoring(original));

    // Make sure the messages don't get messed up
    expectZeroMessages<std::vector<Message>>(&Consumer::runConsumerStatus, consumer);
    expectZeroMessages<std::vector<MessageBringonline>>(&Consumer::runConsumerDeletions, consumer);
    expectZeroMessages<std::vector<MessageBringonline>>(&Consumer::runConsumerStaging, consumer);
    expectZeroMessages<std::map<int, struct MessageLog>>(&Consumer::runConsumerLog, consumer);
    expectZeroMessages<std::vector<MessageUpdater>>(&Consumer::runConsumerStall, consumer);

    // First attempt must return the single message
    std::vector<MessageMonitoring> monitoring;
    BOOST_CHECK_EQUAL(0, consumer.runConsumerMonitoring(monitoring));
    BOOST_CHECK_EQUAL(1, monitoring.size());
    BOOST_CHECK_EQUAL(monitoring[0], original);

    // Second attempt must return empty (already consumed)
    monitoring.clear();
    BOOST_CHECK_EQUAL(0, consumer.runConsumerMonitoring(monitoring));
    BOOST_CHECK_EQUAL(0, monitoring.size());
}


BOOST_FIXTURE_TEST_CASE (simpleStalled, MsgBusFixture)
{
    Producer producer(TEST_PATH);
    Consumer consumer(TEST_PATH);

    MessageUpdater original;

    g_strlcpy(original.job_id, "1906cc40-b915-11e5-9a03-02163e006dd0", sizeof(original.job_id));
    original.file_id = 44;
    original.process_id = 385;
    original.throughput = 0.06;
    original.transferred = 50689;
    g_strlcpy(original.source_surl, "mock://matrioska/thing", sizeof(original.source_surl));
    g_strlcpy(original.dest_surl, "mock://manhattan4/thing", sizeof(original.dest_surl));
    g_strlcpy(original.source_turl, "gsiftp://matrioska/thing", sizeof(original.source_turl));
    g_strlcpy(original.dest_turl, "gsiftp://manhattan4/thing", sizeof(original.dest_turl));
    g_strlcpy(original.transfer_status, "FINISHED", sizeof(original.transfer_status));
    original.msg_errno = 0;

    BOOST_CHECK_EQUAL(0, producer.runProducerStall(original));

    // Make sure the messages don't get messed up
    expectZeroMessages<std::vector<Message>>(&Consumer::runConsumerStatus, consumer);
    expectZeroMessages<std::vector<MessageBringonline>>(&Consumer::runConsumerDeletions, consumer);
    expectZeroMessages<std::vector<MessageBringonline>>(&Consumer::runConsumerStaging, consumer);
    expectZeroMessages<std::map<int, struct MessageLog>>(&Consumer::runConsumerLog, consumer);
    expectZeroMessages<std::vector<MessageMonitoring>>(&Consumer::runConsumerMonitoring, consumer);

    // First attempt must return the single message
    std::vector<MessageUpdater> stalled;
    BOOST_CHECK_EQUAL(0, consumer.runConsumerStall(stalled));
    BOOST_CHECK_EQUAL(1, stalled.size());
    BOOST_CHECK_EQUAL(stalled[0], original);

    // Second attempt must return empty (already consumed)
    stalled.clear();
    BOOST_CHECK_EQUAL(0, consumer.runConsumerStall(stalled));
    BOOST_CHECK_EQUAL(0, stalled.size());
}


BOOST_FIXTURE_TEST_CASE (simpleLog, MsgBusFixture)
{
    Producer producer(TEST_PATH);
    Consumer consumer(TEST_PATH);

    MessageLog original;

    g_strlcpy(original.job_id, "1906cc40-b915-11e5-9a03-02163e006dd0", sizeof(original.job_id));
    original.file_id = 44;
    g_strlcpy(original.host, "abc.cern.ch", sizeof(original.host));
    g_strlcpy(original.filePath, "/var/log/fts3/transfers/log.log", sizeof(original.filePath));
    original.debugFile = true;
    original.msg_errno = 0;

    BOOST_CHECK_EQUAL(0, producer.runProducerLog(original));

    // Make sure the messages don't get messed up
    expectZeroMessages<std::vector<Message>>(&Consumer::runConsumerStatus, consumer);
    expectZeroMessages<std::vector<MessageBringonline>>(&Consumer::runConsumerDeletions, consumer);
    expectZeroMessages<std::vector<MessageBringonline>>(&Consumer::runConsumerStaging, consumer);
    expectZeroMessages<std::vector<MessageMonitoring>>(&Consumer::runConsumerMonitoring, consumer);
    expectZeroMessages<std::vector<MessageUpdater>>(&Consumer::runConsumerStall, consumer);

    // First attempt must return the single message
    std::map<int, struct MessageLog> logs;
    BOOST_CHECK_EQUAL(0, consumer.runConsumerLog(logs));
    BOOST_CHECK_EQUAL(1, logs.size());
    BOOST_CHECK_NO_THROW(logs.at(original.file_id));
    BOOST_CHECK_EQUAL(logs.at(original.file_id), original);

    // Second attempt must return empty (already consumed)
    logs.clear();
    BOOST_CHECK_EQUAL(0, consumer.runConsumerLog(logs));
    BOOST_CHECK_EQUAL(0, logs.size());
}


BOOST_FIXTURE_TEST_CASE (simpleDeletion, MsgBusFixture)
{
    Producer producer(TEST_PATH);
    Consumer consumer(TEST_PATH);

    MessageBringonline original;

    g_strlcpy(original.job_id, "1906cc40-b915-11e5-9a03-02163e006dd0", sizeof(original.job_id));
    original.file_id = 44;
    g_strlcpy(original.transfer_status, "FAILED", sizeof(original.transfer_status));
    g_strlcpy(original.transfer_message, "Could not open because of reasons", sizeof(original.transfer_message));
    original.msg_errno = EPERM;

    BOOST_CHECK_EQUAL(0, producer.runProducerDeletions(original));

    // Make sure the messages don't get messed up
    expectZeroMessages<std::vector<Message>>(&Consumer::runConsumerStatus, consumer);
    expectZeroMessages<std::vector<MessageBringonline>>(&Consumer::runConsumerStaging, consumer);
    expectZeroMessages<std::map<int, struct MessageLog>>(&Consumer::runConsumerLog, consumer);
    expectZeroMessages<std::vector<MessageMonitoring>>(&Consumer::runConsumerMonitoring, consumer);
    expectZeroMessages<std::vector<MessageUpdater>>(&Consumer::runConsumerStall, consumer);

    // First attempt must return the single message
    std::vector<MessageBringonline> statuses;
    BOOST_CHECK_EQUAL(0, consumer.runConsumerDeletions(statuses));
    BOOST_CHECK_EQUAL(1, statuses.size());
    BOOST_CHECK_EQUAL(statuses[0], original);

    // Second attempt must return empty (already consumed)
    statuses.clear();
    BOOST_CHECK_EQUAL(0, consumer.runConsumerDeletions(statuses));
    BOOST_CHECK_EQUAL(0, statuses.size());
}


BOOST_FIXTURE_TEST_CASE (simpleStaging, MsgBusFixture)
{
    Producer producer(TEST_PATH);
    Consumer consumer(TEST_PATH);

    MessageBringonline original;

    g_strlcpy(original.job_id, "1906cc40-b915-11e5-9a03-02163e006dd0", sizeof(original.job_id));
    original.file_id = 44;
    g_strlcpy(original.transfer_status, "FAILED", sizeof(original.transfer_status));
    g_strlcpy(original.transfer_message, "Could not open because of reasons", sizeof(original.transfer_message));
    original.msg_errno = EPERM;

    BOOST_CHECK_EQUAL(0, producer.runProducerStaging(original));

    // Make sure the messages don't get messed up
    expectZeroMessages<std::vector<Message>>(&Consumer::runConsumerStatus, consumer);
    expectZeroMessages<std::vector<MessageBringonline>>(&Consumer::runConsumerDeletions, consumer);
    expectZeroMessages<std::map<int, struct MessageLog>>(&Consumer::runConsumerLog, consumer);
    expectZeroMessages<std::vector<MessageMonitoring>>(&Consumer::runConsumerMonitoring, consumer);
    expectZeroMessages<std::vector<MessageUpdater>>(&Consumer::runConsumerStall, consumer);

    // First attempt must return the single message
    std::vector<MessageBringonline> statuses;
    BOOST_CHECK_EQUAL(0, consumer.runConsumerStaging(statuses));
    BOOST_CHECK_EQUAL(1, statuses.size());
    BOOST_CHECK_EQUAL(statuses[0], original);

    // Second attempt must return empty (already consumed)
    statuses.clear();
    BOOST_CHECK_EQUAL(0, consumer.runConsumerStaging(statuses));
    BOOST_CHECK_EQUAL(0, statuses.size());
}


BOOST_AUTO_TEST_SUITE_END()

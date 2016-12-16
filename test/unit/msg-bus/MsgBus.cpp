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
#include <boost/filesystem.hpp>
#include <boost/function.hpp>

#include <glib.h>

#include "msg-bus/consumer.h"
#include "msg-bus/producer.h"

using namespace fts3::events;


namespace boost {
    bool operator==(const google::protobuf::Message &a, const google::protobuf::Message &b)
    {
        return a.SerializeAsString() == b.SerializeAsString();
    }
}


namespace std {
    std::ostream &operator<<(std::ostream &os, const google::protobuf::Message &msg)
    {
        os << msg.DebugString();
        return os;
    }
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
    original.set_job_id("1906cc40-b915-11e5-9a03-02163e006dd0");
    original.set_transfer_status("FAILED");
    original.set_transfer_message("TEST FAILURE, EVERYTHING IS TERRIBLE");
    original.set_source_se("mock://source/file");
    original.set_dest_se("mock://source/file2");
    original.set_file_id(42);
    original.set_process_id(1234);
    original.set_time_in_secs(55);
    original.set_filesize(1023);
    original.set_nostreams(33);
    original.set_timeout(22);
    original.set_buffersize(1);
    original.set_retry(5);
    original.set_retry(0.2);
    original.set_timestamp(15689);
    original.set_throughput(0.88);

    BOOST_CHECK_EQUAL(0, producer.runProducerStatus(original));

    // Make sure the messages don't get messed up
    expectZeroMessages<std::vector<MessageBringonline>>(&Consumer::runConsumerDeletions, consumer);
    expectZeroMessages<std::vector<MessageBringonline>>(&Consumer::runConsumerStaging, consumer);
    expectZeroMessages<std::map<int, MessageLog>>(&Consumer::runConsumerLog, consumer);
    expectZeroMessages<std::vector<std::string>>(&Consumer::runConsumerMonitoring, consumer);
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

    std::string original = "blah bleh blih bloh cluh";

    BOOST_CHECK_EQUAL(0, producer.runProducerMonitoring(original));

    // Make sure the messages don't get messed up
    expectZeroMessages<std::vector<Message>>(&Consumer::runConsumerStatus, consumer);
    expectZeroMessages<std::vector<MessageBringonline>>(&Consumer::runConsumerDeletions, consumer);
    expectZeroMessages<std::vector<MessageBringonline>>(&Consumer::runConsumerStaging, consumer);
    expectZeroMessages<std::map<int, MessageLog>>(&Consumer::runConsumerLog, consumer);
    expectZeroMessages<std::vector<MessageUpdater>>(&Consumer::runConsumerStall, consumer);

    // First attempt must return the single message
    std::vector<std::string> monitoring;
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

    original.set_job_id("1906cc40-b915-11e5-9a03-02163e006dd0");
    original.set_file_id(44);
    original.set_process_id(385);
    original.set_throughput(0.06);
    original.set_transferred(50689);
    original.set_source_surl("mock://matrioska/thing");
    original.set_dest_surl("mock://manhattan4/thing");
    original.set_source_turl("gsiftp://matrioska/thing");
    original.set_dest_turl("gsiftp://manhattan4/thing");
    original.set_transfer_status("FINISHED");
    original.set_timestamp(time(NULL));

    BOOST_CHECK_EQUAL(0, producer.runProducerStall(original));

    // Make sure the messages don't get messed up
    expectZeroMessages<std::vector<Message>>(&Consumer::runConsumerStatus, consumer);
    expectZeroMessages<std::vector<MessageBringonline>>(&Consumer::runConsumerDeletions, consumer);
    expectZeroMessages<std::vector<MessageBringonline>>(&Consumer::runConsumerStaging, consumer);
    expectZeroMessages<std::map<int, MessageLog>>(&Consumer::runConsumerLog, consumer);
    expectZeroMessages<std::vector<std::string>>(&Consumer::runConsumerMonitoring, consumer);

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

    original.set_job_id("1906cc40-b915-11e5-9a03-02163e006dd0");
    original.set_file_id(44);
    original.set_host("abc.cern.ch");
    original.set_log_path("/var/log/fts3/transfers/log.log");
    original.set_has_debug_file(true);
    original.set_timestamp(time(NULL));

    BOOST_CHECK_EQUAL(0, producer.runProducerLog(original));

    // Make sure the messages don't get messed up
    expectZeroMessages<std::vector<Message>>(&Consumer::runConsumerStatus, consumer);
    expectZeroMessages<std::vector<MessageBringonline>>(&Consumer::runConsumerDeletions, consumer);
    expectZeroMessages<std::vector<MessageBringonline>>(&Consumer::runConsumerStaging, consumer);
    expectZeroMessages<std::vector<std::string>>(&Consumer::runConsumerMonitoring, consumer);
    expectZeroMessages<std::vector<MessageUpdater>>(&Consumer::runConsumerStall, consumer);

    // First attempt must return the single message
    std::map<int, MessageLog> logs;
    BOOST_CHECK_EQUAL(0, consumer.runConsumerLog(logs));
    BOOST_CHECK_EQUAL(1, logs.size());
    BOOST_CHECK_NO_THROW(logs.at(original.file_id()));
    BOOST_CHECK_EQUAL(logs.at(original.file_id()), original);

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

    original.set_job_id("1906cc40-b915-11e5-9a03-02163e006dd0");
    original.set_file_id(44);
    original.set_transfer_status("FAILED");
    original.set_transfer_message("Could not open because of reasons");

    BOOST_CHECK_EQUAL(0, producer.runProducerDeletions(original));

    // Make sure the messages don't get messed up
    expectZeroMessages<std::vector<Message>>(&Consumer::runConsumerStatus, consumer);
    expectZeroMessages<std::vector<MessageBringonline>>(&Consumer::runConsumerStaging, consumer);
    expectZeroMessages<std::map<int, MessageLog>>(&Consumer::runConsumerLog, consumer);
    expectZeroMessages<std::vector<std::string>>(&Consumer::runConsumerMonitoring, consumer);
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

    original.set_job_id("1906cc40-b915-11e5-9a03-02163e006dd0");
    original.set_file_id(44);
    original.set_transfer_status("FAILED");
    original.set_transfer_message("Could not open because of reasons");

    BOOST_CHECK_EQUAL(0, producer.runProducerStaging(original));

    // Make sure the messages don't get messed up
    expectZeroMessages<std::vector<Message>>(&Consumer::runConsumerStatus, consumer);
    expectZeroMessages<std::vector<MessageBringonline>>(&Consumer::runConsumerDeletions, consumer);
    expectZeroMessages<std::map<int, MessageLog>>(&Consumer::runConsumerLog, consumer);
    expectZeroMessages<std::vector<std::string>>(&Consumer::runConsumerMonitoring, consumer);
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

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
#include <boost/filesystem.hpp>
#include <boost/function.hpp>

#include <glib.h>

#include "msg-bus/consumer.h"
#include "msg-bus/producer.h"

using namespace fts3::events;

namespace std {
    std::ostream &operator<<(std::ostream &os, const google::protobuf::Message &msg) {
        os << msg.DebugString();
        return os;
    }
}

class MsgBusFixture : public testing::Test {
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

template<typename MSG_CONTAINER>
static void expectZeroMessages(boost::function<int(Consumer *, MSG_CONTAINER &)> func, Consumer &consumer) {
    MSG_CONTAINER container;
    EXPECT_EQ(0, func(&consumer, container));
    EXPECT_EQ(0, container.size());
}

TEST_F(MsgBusFixture, SimpleStatus) {
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

    EXPECT_EQ(0, producer.runProducerStatus(original));

    // Make sure the messages don't get messed up
    expectZeroMessages<std::vector<MessageBringonline>>(&Consumer::runConsumerDeletions, consumer);
    expectZeroMessages<std::vector<MessageBringonline>>(&Consumer::runConsumerStaging, consumer);
    expectZeroMessages<std::map<int, MessageLog>>(&Consumer::runConsumerLog, consumer);
    expectZeroMessages<std::vector<std::string>>(&Consumer::runConsumerMonitoring, consumer);
    expectZeroMessages<std::vector<MessageUpdater>>(&Consumer::runConsumerStall, consumer);

    // First attempt must return the single message
    std::vector<Message> statuses;
    EXPECT_EQ(0, consumer.runConsumerStatus(statuses));
    EXPECT_EQ(1, statuses.size());
    EXPECT_EQ(statuses[0].SerializeAsString(), original.SerializeAsString());

    // Second attempt must return empty (already consumed)
    statuses.clear();
    EXPECT_EQ(0, consumer.runConsumerStatus(statuses));
    EXPECT_EQ(0, statuses.size());
}

TEST_F(MsgBusFixture, SimpleMonitoring) {
    Producer producer(TEST_PATH);
    Consumer consumer(TEST_PATH);

    std::string original = "blah bleh blih bloh cluh";

    EXPECT_EQ(0, producer.runProducerMonitoring(original));

    // Make sure the messages don't get messed up
    expectZeroMessages<std::vector<Message>>(&Consumer::runConsumerStatus, consumer);
    expectZeroMessages<std::vector<MessageBringonline>>(&Consumer::runConsumerDeletions, consumer);
    expectZeroMessages<std::vector<MessageBringonline>>(&Consumer::runConsumerStaging, consumer);
    expectZeroMessages<std::map<int, MessageLog>>(&Consumer::runConsumerLog, consumer);
    expectZeroMessages<std::vector<MessageUpdater>>(&Consumer::runConsumerStall, consumer);

    // First attempt must return the single message
    std::vector<std::string> monitoring;
    EXPECT_EQ(0, consumer.runConsumerMonitoring(monitoring));
    EXPECT_EQ(1, monitoring.size());
    EXPECT_EQ(monitoring[0], original);

    // Second attempt must return empty (already consumed)
    monitoring.clear();
    EXPECT_EQ(0, consumer.runConsumerMonitoring(monitoring));
    EXPECT_EQ(0, monitoring.size());
}

TEST_F(MsgBusFixture, SimpleLog) {
    Producer producer(TEST_PATH);
    Consumer consumer(TEST_PATH);

    MessageLog original;

    original.set_job_id("1906cc40-b915-11e5-9a03-02163e006dd0");
    original.set_file_id(44);
    original.set_host("abc.cern.ch");
    original.set_log_path("/var/log/fts3/transfers/log.log");
    original.set_has_debug_file(true);
    original.set_timestamp(time(NULL));

    EXPECT_EQ(0, producer.runProducerLog(original));

    // Make sure the messages don't get messed up
    expectZeroMessages<std::vector<Message>>(&Consumer::runConsumerStatus, consumer);
    expectZeroMessages<std::vector<MessageBringonline>>(&Consumer::runConsumerDeletions, consumer);
    expectZeroMessages<std::vector<MessageBringonline>>(&Consumer::runConsumerStaging, consumer);
    expectZeroMessages<std::vector<std::string>>(&Consumer::runConsumerMonitoring, consumer);
    expectZeroMessages<std::vector<MessageUpdater>>(&Consumer::runConsumerStall, consumer);

    // First attempt must return the single message
    std::map<int, MessageLog> logs;
    EXPECT_EQ(0, consumer.runConsumerLog(logs));
    EXPECT_EQ(1, logs.size());
    EXPECT_NO_THROW(logs.at((int) original.file_id()));
    EXPECT_EQ(logs.at((int) original.file_id()).SerializeAsString(), original.SerializeAsString());

    // Second attempt must return empty (already consumed)
    logs.clear();
    EXPECT_EQ(0, consumer.runConsumerLog(logs));
    EXPECT_EQ(0, logs.size());
}

TEST_F(MsgBusFixture, SimpleDeletion) {
    Producer producer(TEST_PATH);
    Consumer consumer(TEST_PATH);

    MessageBringonline original;

    original.set_job_id("1906cc40-b915-11e5-9a03-02163e006dd0");
    original.set_file_id(44);
    original.set_transfer_status("FAILED");
    original.set_transfer_message("Could not open because of reasons");

    EXPECT_EQ(0, producer.runProducerDeletions(original));

    // Make sure the messages don't get messed up
    expectZeroMessages<std::vector<Message>>(&Consumer::runConsumerStatus, consumer);
    expectZeroMessages<std::vector<MessageBringonline>>(&Consumer::runConsumerStaging, consumer);
    expectZeroMessages<std::map<int, MessageLog>>(&Consumer::runConsumerLog, consumer);
    expectZeroMessages<std::vector<std::string>>(&Consumer::runConsumerMonitoring, consumer);
    expectZeroMessages<std::vector<MessageUpdater>>(&Consumer::runConsumerStall, consumer);

    // First attempt must return the single message
    std::vector<MessageBringonline> statuses;
    EXPECT_EQ(0, consumer.runConsumerDeletions(statuses));
    EXPECT_EQ(1, statuses.size());
    EXPECT_EQ(statuses[0].SerializeAsString(), original.SerializeAsString());

    // Second attempt must return empty (already consumed)
    statuses.clear();
    EXPECT_EQ(0, consumer.runConsumerDeletions(statuses));
    EXPECT_EQ(0, statuses.size());
}

TEST_F(MsgBusFixture, SimpleStaging) {
    Producer producer(TEST_PATH);
    Consumer consumer(TEST_PATH);

    MessageBringonline original;

    original.set_job_id("1906cc40-b915-11e5-9a03-02163e006dd0");
    original.set_file_id(44);
    original.set_transfer_status("FAILED");
    original.set_transfer_message("Could not open because of reasons");

    EXPECT_EQ(0, producer.runProducerStaging(original));

    // Make sure the messages don't get messed up
    expectZeroMessages<std::vector<Message>>(&Consumer::runConsumerStatus, consumer);
    expectZeroMessages<std::vector<MessageBringonline>>(&Consumer::runConsumerDeletions, consumer);
    expectZeroMessages<std::map<int, MessageLog>>(&Consumer::runConsumerLog, consumer);
    expectZeroMessages<std::vector<std::string>>(&Consumer::runConsumerMonitoring, consumer);
    expectZeroMessages<std::vector<MessageUpdater>>(&Consumer::runConsumerStall, consumer);

    // First attempt must return the single message
    std::vector<MessageBringonline> statuses;
    EXPECT_EQ(0, consumer.runConsumerStaging(statuses));
    EXPECT_EQ(1, statuses.size());
    EXPECT_EQ(statuses[0].SerializeAsString(), original.SerializeAsString());

    // Second attempt must return empty (already consumed)
    statuses.clear();
    EXPECT_EQ(0, consumer.runConsumerStaging(statuses));
    EXPECT_EQ(0, statuses.size());
}

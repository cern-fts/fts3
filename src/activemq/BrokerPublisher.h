/*
 * Copyright (c) CERN 2013-2025
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

#pragma once

#include <atomic>
#include <list>
#include <decaf/lang/Thread.h>
#include <decaf/lang/Runnable.h>
#include <decaf/util/concurrent/CountDownLatch.h>
#include <decaf/lang/Long.h>
#include <decaf/util/Date.h>
#include <activemq/core/ActiveMQConnectionFactory.h>
#include <activemq/util/Config.h>
#include <activemq/library/ActiveMQCPP.h>
#include <activemq/commands/Command.h>
#include <activemq/transport/TransportListener.h>
#include <cms/Connection.h>
#include <cms/Session.h>
#include <cms/TextMessage.h>
#include <cms/BytesMessage.h>
#include <cms/Message.h>
#include <cms/MapMessage.h>
#include <cms/ExceptionListener.h>
#include <cms/MessageListener.h>
#include <decaf/lang/Exception.h>
#include <decaf/lang/Pointer.h>
#include <condition_variable>
#include <queue>
#include <thread>
#include <pthread.h>
#include "msg-bus/producer.h"
#include "BrokerConfig.h"
#include "common/Logger.h"
#include "msg-bus/consumer.h"
#include "BrokerConnection.h"
#include "msg-bus/DirQ.h"

using namespace fts3::common;

class MessageRemover {
    private:
        std::queue<std::unique_ptr<std::list<std::unique_ptr<MonitoringMessageCallback>>>> messages;
        std::unique_ptr<DirQ> monitoringQueue;

        std::stop_token stop_token;
        std::condition_variable_any cv;
        std::mutex mutex;

    public:

        MessageRemover(const std::string &monitoringDir) : monitoringQueue(std::make_unique<DirQ>(monitoringDir + "/monitoring"))
        {
        }

        ~MessageRemover()
        {
        }

        void start(std::stop_token stoken)
        {
            stop_token = stoken;

            for (;;) {
                std::unique_ptr<std::list<std::unique_ptr<MonitoringMessageCallback>>> list = pop(-1);
                if (!list && stop_token.stop_requested()) {
                    FTS3_COMMON_LOGGER_NEWLOG(DEBUG) << "messageRemoverThread exited!" << commit;
                    return;
                }

                if (list) {
                    for (auto &msg : *list) {
                        if (!dirq_lock(*monitoringQueue, msg->message.dirq_iter.c_str(), 0)) {
                            if (dirq_remove(*monitoringQueue, msg->message.dirq_iter.c_str()) == -1) {
                                FTS3_COMMON_LOGGER_NEWLOG(ERR) << "BrokerPublisher: DirQ lock and remove failed - "
                                                               << msg->message.dirq_iter << commit;
                            }
                        }
                    }
                }
            }
        }

        void push(std::unique_ptr<std::list<std::unique_ptr<MonitoringMessageCallback>>> value)
        {
            std::unique_lock<std::mutex> lock(mutex);
            messages.push(std::move(value));
            lock.unlock();

            cv.notify_all();
        }

        std::unique_ptr<std::list<std::unique_ptr<MonitoringMessageCallback>>> pop(const int wait = -1)
        {
            std::unique_lock<std::mutex> lock(mutex);

            if (messages.empty()) {
                if (wait == 0) {
                    return nullptr;
                } else if (wait > 0) {
                    cv.wait_for(lock, stop_token, std::chrono::seconds(wait), [this] {return !messages.empty();});
                } else if (wait == -1) {
                    cv.wait(lock, stop_token, [this] {return !messages.empty();});
                }

                if (messages.empty()) {
                    return nullptr;
                }
            }

            std::unique_ptr<std::list<std::unique_ptr<MonitoringMessageCallback>>> ret(std::move(messages.front()));
            messages.pop();
            lock.unlock();

            cv.notify_all();
            return ret;
        }
};

class BrokerPublisher {
    private:
        //ActiveMQ Brokers
        std::list<BrokerConnection> MsgProducers;
        std::list<BrokerConnection>::const_iterator current_producer;

        std::list<std::unique_ptr<MonitoringMessageCallback>> messages;
        std::unique_ptr<std::list<std::unique_ptr<MonitoringMessageCallback>>> messages_to_remove;

        void dispatch_messages();
        std::stop_token stop_token;

        const BrokerConfig& brokerConfig;
        MessageRemover& msgRemover;

        bool refresh_sessions();
        void collect_messages();

        bool connected;

    public:
        BrokerPublisher(const BrokerConfig &config, MessageRemover& msgRemover, std::stop_token token);
        ~BrokerPublisher();

        void start();
        bool sendMessage(MonitoringMessageCallback &msg);
};

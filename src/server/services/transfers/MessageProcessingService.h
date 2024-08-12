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

#pragma once
#ifndef PROCESSQUEUE_H_
#define PROCESSQUEUE_H_

#include <vector>

#include "msg-bus/consumer.h"
#include "msg-bus/producer.h"
#include "server/common/BaseService.h"

namespace fts3 {
namespace server {


class MessageProcessingService: public BaseService
{
public:
    MessageProcessingService();
    virtual ~MessageProcessingService() = default;

    virtual void runService();

private:
    /// Handle only messages whose message state is UPDATE.
    /// These messages are usually sent to update certain fields such as filesize.
    void handleUpdateMessages(const std::vector<fts3::events::Message>& messages);

    /// Handle all messages except for UPDATE ones.
    /// Normally, these messages change the file and job status.
    void handleOtherMessages(const std::vector<fts3::events::Message>& messages);

    /// Perform the database change associated with an UPDATE type message
    void performUpdateMessageDbChange(const fts3::events::Message& msg);
    /// Perform the database change associated with a non-UPDATE type message
    void performOtherMessageDbChange(const fts3::events::Message& msg);

    /// Dump the messages and messages logs onto disk
    void dumpMessages();

    /// Return whether an error message cannot be recovered from
    bool isUnrecoverableErrorMessage(const std::string& errmsg);

    std::vector<fts3::events::Message> messages;
    std::map<int, fts3::events::MessageLog> messagesLog;
    std::vector<fts3::events::MessageUpdater> messagesUpdater;

    Consumer consumer;
    Producer producer;
};

} // end namespace server
} // end namespace fts3


#endif // PROCESSQUEUE_H_

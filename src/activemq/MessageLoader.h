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

#include <thread>
#include "msg-bus/DirQ.h"

class MessageLoader
{
    private:
        std::string last_message = "00000000000000";
        std::unique_ptr<DirQ> monitoringQueue;
        std::stop_token stop_token;
        //_purge(monitoringQueue.get());

        int loadMonitoringMessages();

    public:
        MessageLoader(const std::string &baseDir, std::stop_token stop_token);
        ~MessageLoader();

        void start();
};

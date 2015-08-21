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
#ifndef FILEMONITOR_H_
#define FILEMONITOR_H_

#include <string>
#include <boost/thread.hpp>

namespace fts3 {
namespace config {

class ServerConfig;

/**
 * This class monitors in a background thread if the FTS3 configuration file
 * has been modified. If it has, it triggers a reload of the configuration
 */
class FileMonitor
{

public:
    FileMonitor(ServerConfig* sc);
    virtual ~FileMonitor();

    void start(std::string path);
    void stop();

    static void run (FileMonitor* const me);

private:
    ServerConfig* sc;

    std::string path;
    bool running;

    std::unique_ptr<boost::thread> monitor_thread;

    time_t timestamp;
};

} // end namespace config
} // end namespace fts3

#endif // FILEMONITOR_H_

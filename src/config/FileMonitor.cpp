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

#include "FileMonitor.h"
#include <sys/stat.h>
#include "common/Logger.h"
#include "ServerConfig.h"

using namespace fts3::common;
using namespace fts3::config;


FileMonitor::FileMonitor(ServerConfig* sc) : sc(sc), running(false), timestamp(0)
{
    FTS3_COMMON_LOGGER_NEWLOG(TRACE) << "FileMonitor created" << commit;
}


FileMonitor::~FileMonitor()
{
    // stop the monitoring thread
    if (monitor_thread.get()) {
        running = false;
        monitor_thread->interrupt();
        monitor_thread->join();
    }
    FTS3_COMMON_LOGGER_NEWLOG(TRACE) << "FileMonitor destroyed" << commit;
}


void FileMonitor::start(std::string path)
{
    // check if it's already running
    if (running) return;
    // set the running state
    running = true;
    // set the path to the file
    this->path = path;

    // check the timestamp
    struct stat st;
    if (stat (path.c_str(), &st) == 0)
        timestamp = st.st_mtime;
    else
        timestamp = time(NULL);

    // start monitoring thread
    monitor_thread.reset (
        new boost::thread(run, this)
    );
}


void FileMonitor::stop()
{
    // stop the monitoring thread
    running = false;
}


void FileMonitor::run(FileMonitor *const me)
{
    struct stat st;

    // monitor the file
    while (me->running) {
        // we will check the timestamp periodically every minute
        boost::this_thread::sleep(boost::posix_time::seconds(60));
        // check the timestamp
        if (stat(me->path.c_str(), &st) == 0) {
            time_t new_timestamp = st.st_mtime;
            // compare with the old one
            if (new_timestamp != me->timestamp) {
                // if the file has been changed reload the configuration
                me->timestamp = new_timestamp;
                me->sc->read(0, 0);
            }
        }
    }
}

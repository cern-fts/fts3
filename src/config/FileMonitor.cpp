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


FileMonitor::FileMonitor(ServerConfig* sc) : sc(sc), timestamp(0)
{
    FTS3_COMMON_LOGGER_NEWLOG(TRACE) << "FileMonitor created" << commit;
}


FileMonitor::~FileMonitor()
{
    stop();
    FTS3_COMMON_LOGGER_NEWLOG(TRACE) << "FileMonitor destroyed" << commit;
}


void FileMonitor::start(std::string path)
{
    if (monitor_thread.get()) {
        return;
    }

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
    if (monitor_thread.get()) {
        monitor_thread->interrupt();
        monitor_thread->join();
        monitor_thread.reset(NULL);
    }
}


void FileMonitor::run(FileMonitor *const me)
{
    struct stat st;

    try {
        while (!boost::this_thread::interruption_requested()) {
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
    catch (const boost::thread_interrupted&) {
        return;
    }
    catch (const std::exception &e) {
        FTS3_COMMON_LOGGER_NEWLOG(WARNING) << "FileMonitor thread exited with error " << e.what() << commit;
    }
    catch (...) {
        FTS3_COMMON_LOGGER_NEWLOG(WARNING) << "FileMonitor thread exited with unknwon error" << commit;
    }
}

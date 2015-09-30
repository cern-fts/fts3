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

#include "Profiler.h"

#include "../config/ServerConfig.h"
#include "common/Logger.h"
#include "db/generic/SingleDbInstance.h"

using namespace fts3;
using namespace fts3::common;


/**
 * Current timestamp in milliseconds
 */
inline double getMilliseconds()
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return static_cast<double>(ts.tv_nsec) / 1000000 +
           static_cast<double>(ts.tv_sec) * 1000;
}



ScopeProfiler::ScopeProfiler(const std::string& scope):
    scope(scope), nExceptions(0)
{
    start = getMilliseconds();
}



ScopeProfiler::~ScopeProfiler()
{
    double end = getMilliseconds();
    Profile& prof = ProfilingSubsystem::instance().getProfile(scope);

    std::lock_guard<std::mutex> lock(prof.mutex);

    ++prof.nCalled;
    prof.nExceptions += nExceptions;
    prof.totalTime   += end - start;
}



void ScopeProfiler::exception()
{
    ++nExceptions;
}



void profilerThread(ProfilingSubsystem* profSubsys)
{
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Profiling subsystem started with an interval of "
                                    << profSubsys->getInterval() << commit;
    while (1)
        {
            boost::this_thread::sleep(boost::posix_time::seconds(profSubsys->getInterval()));
            FTS3_COMMON_LOGGER_NEWLOG(DEBUG) << '\n' << *profSubsys << commit;

            try
                {
                    db::DBSingleton::instance().getDBObjectInstance()->storeProfiling(profSubsys);
                }
            catch (std::exception& e)
                {
                    FTS3_COMMON_LOGGER_NEWLOG(ERR) << e.what() << commit;
                }
            catch (...)
                {
                    FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Unexpected exception in profilerThread" << commit;
                }

            // Reset
            profSubsys->clear();
        }
}



ProfilingSubsystem::ProfilingSubsystem(): dumpInterval(0)
{
}



ProfilingSubsystem::~ProfilingSubsystem()
{

}



void ProfilingSubsystem::start()
{
    dumpInterval = config::ServerConfig::instance().get<unsigned>("Profiling");
    if (dumpInterval)
        {
            boost::thread btUpdater(profilerThread, this);
        }
}


Profile& ProfilingSubsystem::getProfile(const std::string& scope)
{
    // Note: scope may not be in profiles, so this may modify the map,
    // in which case it needs to lock
    std::lock_guard<std::mutex> lock(mutex);
    return profiles[scope];
}



unsigned ProfilingSubsystem::getInterval() const
{
    return dumpInterval;
}


std::map<std::string, Profile> ProfilingSubsystem::getProfiles() const
{
    std::lock_guard<std::mutex> lock(mutex);
    return profiles;
}


void ProfilingSubsystem::clear()
{
    std::lock_guard<std::mutex> lock(mutex);
    profiles.clear();
}


std::ostream& fts3::operator << (std::ostream& out, const Profile& prof)
{
    std::lock_guard<std::mutex> lock(prof.mutex);

    out << "executed " << std::setw(3) << std::dec << prof.nCalled << " times, "
        <<  "thrown " << std::setw(3) << prof.nExceptions << " exceptions, average of "
        << prof.getAverage() << "ms";
    return out;
}



std::ostream& fts3::operator << (std::ostream& out, const ProfilingSubsystem& profSubsys)
{
    std::lock_guard<std::mutex> lock(profSubsys.mutex);

    std::map<std::string, Profile>::const_iterator i;
    for (i = profSubsys.profiles.begin(); i != profSubsys.profiles.end(); ++i)
        {
            out << "PROFILE: " << std::setw(32) << i->first << " - " << i->second << std::endl;
        }
    return out;
}

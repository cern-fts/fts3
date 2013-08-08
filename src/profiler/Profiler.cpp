/********************************************
 * Copyright @ Members of the EMI Collaboration, 2013.
 * See www.eu-emi.eu for details on the copyright holders.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 ***********************************************/

#include "Profiler.h"
#include "config/serverconfig.h"
#include "common/logger.h"
#include "SingleDbInstance.h"

using namespace fts3;
using namespace fts3::common;

ProfilingSubsystem ProfilingSubsystem::instance;

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
    Profile& prof = ProfilingSubsystem::getInstance().getProfile(scope);

    boost::mutex::scoped_lock lock(prof.mutex);

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
    while (1) {
        boost::this_thread::sleep(boost::posix_time::seconds(profSubsys->getInterval()));
        FTS3_COMMON_LOGGER_NEWLOG(DEBUG) << '\n' << *profSubsys << commit;

        try {
            db::DBSingleton::instance().getDBObjectInstance()->storeProfiling(profSubsys);
        }
        catch (std::exception& e) {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << e.what() << commit;
        }
        catch (...) {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Unexpected exception in profilerThread" << commit;
        }

        // Reset
        profSubsys->profiles.clear();
    }
}



ProfilingSubsystem::ProfilingSubsystem(): dumpInterval(0)
{
}



ProfilingSubsystem::~ProfilingSubsystem()
{

}



ProfilingSubsystem& ProfilingSubsystem::getInstance()
{
    return ProfilingSubsystem::instance;
}


void ProfilingSubsystem::start()
{
    dumpInterval = fts3::config::theServerConfig().get<unsigned>("Profiling");
    if (dumpInterval) {
        boost::thread btUpdater(profilerThread, this);
    }
}


Profile& ProfilingSubsystem::getProfile(const std::string& scope)
{
    return profiles[scope];
}



unsigned ProfilingSubsystem::getInterval() const
{
    return dumpInterval;
}



std::ostream& fts3::operator << (std::ostream& out, const Profile& prof)
{
    out << "executed " << std::setw(3) << prof.nCalled << " times, "
        <<  "thrown " << std::setw(3) << prof.nExceptions << " exceptions, average of "
        << prof.getAverage() << "ms";
    return out;
}



std::ostream& fts3::operator << (std::ostream& out, const ProfilingSubsystem& profSubsys) {
    std::map<std::string, Profile>::const_iterator i;
    for (i = profSubsys.profiles.begin(); i != profSubsys.profiles.end(); ++i) {
        out << "PROFILE: " << std::setw(32) << i->first << " - " << i->second << std::endl;
    }
    return out;
}

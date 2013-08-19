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

#pragma once

#include <boost/thread/mutex.hpp>
#include <iomanip>
#include <map>
#include <ostream>
#include <string>

namespace fts3
{


/**
 * Profiling data
 */
struct Profile
{
    boost::mutex    mutex;

    unsigned long   nCalled;
    unsigned long   nExceptions;
    double          totalTime;

    Profile(): nCalled(0), nExceptions(0), totalTime(0)
    {
    }

    Profile(const Profile& o): nCalled(o.nCalled), nExceptions(o.nExceptions),
        totalTime(o.totalTime)
    {
    }

    double getAverage() const
    {
        if (nCalled)
            return totalTime / static_cast<double>(nCalled);
        else
            return 0;
    }
};

std::ostream& operator << (std::ostream& out, const Profile& prof);

/**
 * Profiles a scope, using constructor/destructor to start/finish
 * the profiling
 */
class ScopeProfiler
{
private:
    std::string scope;
    double      start;
    unsigned    nExceptions;

public:
    ScopeProfiler(const std::string& scope);
    ~ScopeProfiler();

    void exception();
};


/**
 * Profiler singleton. Aggregates the profiling data.
 */
class ProfilingSubsystem
{
private:
    static ProfilingSubsystem instance;

    unsigned dumpInterval;

public:
    static ProfilingSubsystem& getInstance();

    std::map<std::string, Profile> profiles;

    ProfilingSubsystem();
    ~ProfilingSubsystem();

    void start();

    Profile& getProfile(const std::string &scope);
    unsigned getInterval() const;

};

std::ostream& operator << (std::ostream& out, const ProfilingSubsystem& profSubsys);

}

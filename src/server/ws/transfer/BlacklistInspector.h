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

#ifndef BLACKLISTINSPECTOR_H_
#define BLACKLISTINSPECTOR_H_

#include "db/generic/SingleDbInstance.h"

#include <string>
#include <set>
#include <map>

namespace fts3
{
namespace ws
{

using namespace db;

class BlacklistInspector
{
    class TimeoutAssigner
    {

    public:

        TimeoutAssigner(std::map<std::string, int>& timeouts) : timeouts(timeouts) {}

        template<typename T>
        void operator ()(T& t)
        {
            std::string const & src = t.source_se;
            std::string const & dst = t.dest_se;
            // if theres's nothing to do continue
            if (timeouts.find(src) == timeouts.end() && timeouts.find(dst) == timeouts.end()) return;
            // if src has no timeout use destination and continue
            if (timeouts.find(src) == timeouts.end())
                {
                    t.wait_timeout = timeouts[dst];
                    return;
                }
            // if dst has no time use source
            if (timeouts.find(dst) == timeouts.end())
                {
                    t.wait_timeout = timeouts[src];
                    return;
                }
            // if both dst and src have timeout pick the lower value
            t.wait_timeout = timeouts[src] < timeouts[dst] ? timeouts[src] : timeouts[dst];
        }

    private:

        std::map<std::string, int> timeouts;
    };

public:

    BlacklistInspector(std::string const & vo) : db (DBSingleton::instance().getDBObjectInstance()), vo(vo) {}
    virtual ~BlacklistInspector() {}

    void add(std::string const & se);

    void inspect() const;
    void setWaitTimeout(std::list<JobElementTuple> & jobs) const;

private:

    /// DB instance
    GenericDbIfce* db;

    std::set<std::string> unique_ses;
    std::string const & vo;
    std::string unique_ses_str;
};

} /* namespace cli */
} /* namespace fts3 */

#endif /* BLACKLISTINSPECTOR_H_ */

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

#include "BlacklistInspector.h"

#include <list>
#include <algorithm>
#include <numeric>

#include <boost/lambda/lambda.hpp>
#include "../../../../../common/Exceptions.h"

namespace fts3
{
namespace ws
{

using namespace fts3::common;

void BlacklistInspector::add(std::string const & se)
{
    unique_ses.insert(se);
}

void BlacklistInspector::inspect() const
{
    // get the list of SEs that are blacklisted
    std::list<std::string> notAllowed;
    for (auto i = unique_ses.begin(); i != unique_ses.end(); ++i) {
        if (!db->allowSubmit(*i, vo))
            notAllowed.push_back(*i);
    }

    // if all the SEs are OK there's nothing to do ...
    if (notAllowed.empty()) return;

    // accumulate all the blacklisted SEs in one string, comma seperated
    std::string notAllowedStr = std::accumulate(
                                    notAllowed.begin(), notAllowed.end(), std::string(),
                                    boost::lambda::_1 + boost::lambda::_2 + ","
                                );
    // erase the leading comma
    notAllowedStr.resize(notAllowedStr.size() - 1);

    // throw an exception ...
    throw UserError("Following SEs: " + notAllowedStr + " are blacklisted!");
}

void BlacklistInspector::setWaitTimeout(std::list<SubmittedTransfer> & jobs) const
{
    // get the timeouts from DB
    std::map<std::string, int> timeouts;
    for (auto i = unique_ses.begin(); i != unique_ses.end(); ++i) {
        boost::optional<int> timeout = db->getTimeoutForSe(*i);
        if (timeout) {
            timeouts[*i] = timeout.get();
        }
    }
    // create the assigner object
    TimeoutAssigner assign_timeout(timeouts);
    // fill in wait timeouts
    std::for_each(jobs.begin(), jobs.end(), assign_timeout);
}

} /* namespace cli */
} /* namespace fts3 */

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

#include "TransferCreator.h"

#include <map>
#include <algorithm>
#include <iterator>
#include "../../../../../common/Exceptions.h"

namespace fts3
{
namespace ws
{

using namespace fts3::common;

TransferCreator::~TransferCreator()
{

}

std::list< boost::tuple<std::string, std::string, std::string, int> > TransferCreator::pairSourceAndDestination(
    std::vector<std::string> const & sources,
    std::vector<std::string> const & destinations
)
{
    std::list< boost::tuple<std::string, std::string, std::string, int> > ret;

    // if it is single source - single destination submission just return the pair
    if (sources.size() == 1 && destinations.size() == 1)
        {
            ret.push_back(
                boost::make_tuple(sources.front(), destinations.front(), initialState, fileIndex)
            );
            return ret;
        }

    // it is a multi-replica - single destination submission
    // pick just one for submission, rest will be in NOT_USED state
    if (sources.size() > 1 && destinations.size() == 1)
        {
            // construct N to 1 transformation
            to_transfer<1> Nto1(*destinations.begin(), "NOT_USED", fileIndex);
            // create the transfers
            std::transform(sources.begin(), sources.end(), std::inserter(ret, ret.begin()), Nto1);
            // set the first one to initial state
            boost::get<2>(*ret.begin()) = initialState;

            return ret;
        }

    // it is single source - multiple destination replication job
    if (sources.size() == 1 && destinations.size() > 1)
        {
            // construct 1 to N transformation
            // bare in mind that each transfer file will have a different file index
            to_transfer<2, 1> _1toN(*sources.begin(), initialState, fileIndex);
            // create the transfers
            std::transform(destinations.begin(), destinations.end(), std::inserter(ret, ret.begin()), _1toN);

            return ret;
        }

    // and finally if it is multi source - multi destination job
    // it has to contain different protocols

    std::map<std::string, std::string> protocol_srcs, protocol_dsts;

    // map sources using their protocols
    std::transform(sources.begin(), sources.end(), std::inserter(protocol_srcs, protocol_srcs.begin()), map_protocol);
    // map destinations using their protocols
    std::transform(destinations.begin(), destinations.end(), std::inserter(protocol_dsts, protocol_dsts.begin()), map_protocol);

    // find a match for each source
    std::map<std::string, std::string>::iterator it_p;
    for (it_p = protocol_srcs.begin(); it_p != protocol_srcs.end(); ++it_p)
        {
            std::string protocol = it_p->first;
            if (protocol_dsts.find(protocol) == protocol_dsts.end()) continue;

            std::string src = it_p->second;
            std::string dst = protocol_dsts[protocol];

            ret.push_back(
                boost::make_tuple(src, dst, "NOT_USED", fileIndex)
            );
        }

    if (ret.empty()) throw UserError("It has not been possible to pair the sources with destinations (protocols don't match)!");

    // set the first one to initial sate
    boost::get<2>(*ret.begin()) = initialState;

    return ret;
}

} /* namespace cli */
} /* namespace fts3 */

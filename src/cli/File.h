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

#ifndef TRANSFERJOB_H_
#define TRANSFERJOB_H_

#include <string>
#include <vector>
#include <boost/optional/optional.hpp>

namespace fts3
{
namespace cli
{

/**
 * Job element (single file)
 */
struct File
{

    File () {}

    File (
        std::vector<std::string> const & s,
        std::vector<std::string> const & d,
        boost::optional<std::string> const & c = boost::none,
        boost::optional<double> const & fs = boost::none,
        boost::optional<std::string> const & m = boost::none,
        boost::optional<std::string> const & ss = boost::none,
        boost::optional<std::string> const & a = boost::none
    ) : sources(s), destinations(d), selection_strategy(ss), checksum(c), file_size(fs), metadata(m), activity(a)
    {

    }

    /// the source files (replicas)
    std::vector<std::string> sources;
    /// the destination files (the same SE different protocols)
    std::vector<std::string> destinations;
    /// source selection strategy
    boost::optional<std::string> selection_strategy;
    /// checksum
    boost::optional<std::string> checksum;
    /// file size
    boost::optional<double> file_size;
    /// metadata
    boost::optional<std::string> metadata;
    /// activity
    boost::optional<std::string> activity;
};

}
}

#endif /* TRANSFERJOB_H_ */

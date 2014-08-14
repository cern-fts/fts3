/*
 *	Copyright notice:
 *	Copyright © Members of the EMI Collaboration, 2010.
 *
 *	See www.eu-emi.eu for details on the copyright holders
 *
 *	Licensed under the Apache License, Version 2.0 (the "License");
 *	you may not use this file except in compliance with the License.
 *	You may obtain a copy of the License at
 *
 *		http://www.apache.org/licenses/LICENSE-2.0
 *
 *	Unless required by applicable law or agreed to in writing, software
 *	distributed under the License is distributed on an "AS IS" BASIS,
 *	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *	See the License for the specific language governing permissions and
 *	limitations under the License.
 *
 * TransferJob.h
 *
 *  Created on: Sep 7, 2012
 *      Author: simonm
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
        std::vector<std::string> s,
        std::vector<std::string> d,
        std::vector<std::string> c = std::vector<std::string>(),
        boost::optional<double> fs = boost::optional<double>(),
        boost::optional<std::string> m = boost::optional<std::string>(),
        boost::optional<std::string> ss = boost::optional<std::string>())
    {

        sources = s;
        destinations = d;
        checksums = c;
        file_size = fs;
        metadata = m;
        selection_strategy = ss;
    }

    /// the source files (replicas)
    std::vector<std::string> sources;
    /// the destination files (the same SE different protocols)
    std::vector<std::string> destinations;
    /// source selection strategy
    boost::optional<std::string> selection_strategy;
    /// checksum (multiple checksums in case of protocols that don't support adler32)
    std::vector<std::string> checksums;
    /// file size
    boost::optional<double> file_size;
    /// metadata
    boost::optional<std::string> metadata;
};

}
}

#endif /* TRANSFERJOB_H_ */

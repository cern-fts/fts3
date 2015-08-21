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

#ifndef RESPONSEPARSER_H_
#define RESPONSEPARSER_H_

#include "JobStatus.h"
#include "Snapshot.h"

#include <istream>
#include <string>
#include <vector>

#include <boost/property_tree/ptree.hpp>

namespace fts3
{

namespace cli
{

namespace pt = boost::property_tree;

class ResponseParser
{

public:

    ResponseParser(std::istream& stream);
    ResponseParser(std::string const & json);

    virtual ~ResponseParser();

    std::string get(std::string const & path) const;

    std::vector<JobStatus> getJobs(std::string const & path) const;

    std::vector<FileInfo> getFiles(std::string const & path) const;

    int getNb(std::string const & path, std::string const & state) const;

    std::vector<Snapshot> getSnapshot(bool rest = true) const;

    std::vector<DetailedFileStatus> getDetailedFiles(std::string const & path) const;

private:

    std::vector<Snapshot> get_snapshot_for_rest() const;
    std::vector<Snapshot> get_snapshot_for_soap() const;

    /// The object that contains the response
    pt::ptree response;
};

}
}
#endif /* RESPONSEPARSER_H_ */

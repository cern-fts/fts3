/*
 * ResponseParser.h
 *
 *  Created on: Feb 21, 2014
 *      Author: simonm
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

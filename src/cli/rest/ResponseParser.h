/*
 * ResponseParser.h
 *
 *  Created on: Feb 21, 2014
 *      Author: simonm
 */

#ifndef RESPONSEPARSER_H_
#define RESPONSEPARSER_H_

#include "JobStatus.h"

#include <istream>
#include <string>

#include <boost/property_tree/ptree.hpp>

namespace fts3
{

namespace cli
{

using namespace boost::property_tree;

class ResponseParser
{

public:

    ResponseParser(std::istream& stream);

    virtual ~ResponseParser();

    std::string get(std::string const & path) const;

    std::vector<JobStatus> getJobs(std::string const & path) const;

private:
    /// The object that contains the response
    ptree response;
};

}
}
#endif /* RESPONSEPARSER_H_ */

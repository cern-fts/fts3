/*
 * ResponseParser.h
 *
 *  Created on: Feb 21, 2014
 *      Author: simonm
 */

#ifndef RESPONSEPARSER_H_
#define RESPONSEPARSER_H_

#include "TransferTypes.h"

#include <istream>
#include <string>

#include <boost/property_tree/ptree.hpp>

namespace fts3
{

namespace cli
{

using namespace std;
using namespace boost::property_tree;

class ResponseParser
{

public:

    ResponseParser(istream& stream);

    virtual ~ResponseParser();

    string get(string const & path) const;

    vector<JobStatus> getJobs(string const & path) const;

private:
    /// The object that contains the response
    ptree response;
};

}
}
#endif /* RESPONSEPARSER_H_ */

/*
 * RestModifyJob.h
 *
 *  Created on: 13 Nov 2015
 *      Author: dhsmith
 */

#ifndef RESTMODIFYJOB_H_
#define RESTMODIFYJOB_H_

#include <vector>
#include <string>
#include <map>
#include <ostream>

#include "HttpRequest.h"
#include "ResponseParser.h"

#include <boost/property_tree/ptree.hpp>

namespace fts3
{
namespace cli
{

namespace pt = boost::property_tree;

class RestModifyJob
{

public:
    // only modifies the priority
    RestModifyJob(std::string jobId, int priority);
    virtual ~RestModifyJob();

    std::string body() const;
    std::string resource() const;
    void do_http_action(HttpRequest &http) const;

protected:
    pt::ptree bodyjson;
    std::string jobId;
};

} /* namespace cli */
} /* namespace fts3 */

#endif /* RESTMODIFYJOB_H_ */

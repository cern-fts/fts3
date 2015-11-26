/*
 * RestBanning.h
 *
 *  Created on: 10 Nov 2015
 *      Author: dhsmith
 */

#ifndef RESTBANNING_H_
#define RESTBANNING_H_

#include <vector>
#include <string>
#include <map>
#include <ostream>

#include "HttpRequest.h"

#include <boost/property_tree/ptree.hpp>

namespace fts3
{
namespace cli
{

namespace pt = boost::property_tree;

class RestBanning
{

public:
    RestBanning(std::string name, std::string vo, std::string status, int timeout, bool mode, bool isuser);
    virtual ~RestBanning();

    std::string body() const;
    std::string resource() const;
    void do_http_action(HttpRequest &http) const;

protected:
    pt::ptree bodyjson;
    std::string name;
    bool mode;
    bool isUser;
};

} /* namespace cli */
} /* namespace fts3 */

#endif /* RESTBANNING_H_ */

/*
 * RestContextAdapter.h
 *
 *  Created on: 15 Aug 2014
 *      Author: simonm
 */

#ifndef RESTCONTEXTADAPTER_H_
#define RESTCONTEXTADAPTER_H_

#include "ServiceAdapter.h"
#include "JobStatus.h"

#include <string>
#include <vector>

namespace fts3 {
namespace cli {

class RestContextAdapter : public ServiceAdapter
{

public:
    RestContextAdapter(std::string const & endpoint, std::string const & capath, std::string const & proxy):
        ServiceAdapter(endpoint),
        capath(capath),
        proxy(proxy) {}

    virtual ~RestContextAdapter() {}

    std::vector<JobStatus> listRequests (std::vector<std::string> const & statuses, std::string const & dn, std::string const & vo, std::string const & source, std::string const & destination);

private:

    void getInterfaceDeatailes();

    std::string const capath;
    std::string const proxy;
};

} /* namespace cli */
} /* namespace fts3 */

#endif /* RESTCONTEXTADAPTER_H_ */

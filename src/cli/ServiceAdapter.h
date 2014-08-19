/*
 * ServiceAdapter.h
 *
 *  Created on: 18 Aug 2014
 *      Author: simonm
 */

#ifndef SERVICEADAPTER_H_
#define SERVICEADAPTER_H_

#include "JobStatus.h"

#include <string>
#include <vector>

namespace fts3 {
namespace cli {

class ServiceAdapter
{

public:
    ServiceAdapter(std::string const & endpoint) : endpoint(endpoint) {}
    virtual ~ServiceAdapter() {}

    void printServiceDetails();

    /**
     * Remote call to listRequests
     * Internally is listRequests2
     *
     * @param dn user dn
     * @param vo vo name
     * @param array statuses of interest
     * @param resp server response
     */
    virtual std::vector<JobStatus> listRequests (std::vector<std::string> const & statuses, std::string const & dn, std::string const & vo, std::string const & source, std::string const & destination) = 0;

    /**
     * Remote call to cancel
     *
     * @param jobIds : list of job IDs
     */
    virtual std::vector< std::pair<std::string, std::string>  > cancel(std::vector<std::string> const & jobIds) = 0;

protected:

    virtual void getInterfaceDeatailes() = 0;

    std::string const endpoint;

    ///@{
    /**
     * general informations about the FTS3 service
     */
    std::string interface;
    std::string version;
    std::string schema;
    std::string metadata;
    ///@}
};

} /* namespace cli */
} /* namespace fts3 */

#endif /* SERVICEADAPTER_H_ */

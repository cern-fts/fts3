/*
 * ServiceAdapter.h
 *
 *  Created on: 18 Aug 2014
 *      Author: simonm
 */

#ifndef SERVICEADAPTER_H_
#define SERVICEADAPTER_H_

#include "JobStatus.h"
#include "File.h"

#include <string>
#include <vector>
#include <map>

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

    /**
     * Remote call that will be transferSubmit2 or transferSubmit3
     *
     * @param elements - job elements to be executed
     * @param parameters - parameters for the job that is being submitted
     * @param checksum - flag indicating whether the checksum should be used
     *  (if false transferSubmit2 is used, otherwise transferSubmit3 is used)
     *
     * @return the job ID
     */
    virtual std::string transferSubmit (std::vector<File> const & files, std::map<std::string, std::string> const & parameters) = 0;

    /**
     * Remote call to getTransferJobStatus
     *
     * @param jobId   the job id
     * @param archive if true, the archive will be queried
     *
     * @return an object holding the job status
     */
    virtual JobStatus getTransferJobStatus (std::string const & jobId, bool archive) = 0;

    /**
     * Remote call to getTransferJobSummary
     * Internally it is getTransferJobSummary3
     *
     * @param jobId   id of the job
     * @param archive if true, the archive will be queried
     *
     * @return an object containing job summary
     */
    virtual JobStatus getTransferJobSummary (std::string const & jobId, bool archive) = 0;

    /**
     * Remote call to getFileStatus
     *
     * @param jobId   id of the job
     * @param archive if true, the archive will be queried
     * @param offset  query starting from this offset (i.e. files 100 in advance)
     * @param limit   query a limited number of files (i.e. only 50 results)
     * @param retries get file retries
     * @param resp server response
     * @return The number of files returned
     */
    virtual std::vector<FileInfo> getFileStatus (std::string const & jobId, bool archive, int offset, int limit, bool retries) = 0;

    /**
     * Delegates a proxy certificate
     *
     * @param delegationId : delegation ID
     * @param expirationTime : user defined expiration time
     */
    virtual void delegate(std::string const & delegationId, long expirationTime) = 0;

    /**
     * @param jobId : job ID
     * @return : vector containing detailed information about files in the given job (including file ID)
     */
    virtual std::vector<DetailedFileStatus> getDetailedJobStatus(std::string const & jobId) = 0;

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

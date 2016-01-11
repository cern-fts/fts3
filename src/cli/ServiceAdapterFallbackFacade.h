/*
 * ServiceAdapterFallbackFacade.h
 *
 *  Created on: 14 Oct 2015
 *      Author: dhsmith
 */

#ifndef SERVICEADAPTERFALLBACKFACADE_H_
#define SERVICEADAPTERFALLBACKFACADE_H_

#include "ServiceAdapter.h"

#include "exception/cli_exception.h"

#include "Snapshot.h"
#include "JobStatus.h"
#include "File.h"

#include <boost/tuple/tuple.hpp>

#include <string>
#include <vector>
#include <utility>
#include <signal.h>

namespace fts3
{
namespace cli
{

/**
 * An adapter class presenting a facade behind which with the GSoap or Rest adapters are used.
 * In some cases a fallback algorithm is used to try GSoap after having failed to use Rest.
 *	
 * Facades all the functionalities of transfer and configuration web service
 */
class ServiceAdapterFallbackFacade : public ServiceAdapter
{

public:
    ServiceAdapterFallbackFacade(std::string const & endpoint, std::string const & capath = std::string(), std::string const & proxy = std::string()):
        ServiceAdapter(endpoint),
        capath(capath),
        proxy(proxy),
        fbstate(RESTWITHFB) {}

    ServiceAdapterFallbackFacade(const ServiceAdapterFallbackFacade &other);

    virtual ~ServiceAdapterFallbackFacade() {}

    void authorize(const std::string& op, const std::string& dn);
    void blacklistDn(std::string subject, std::string status, int timeout, bool mode);
    void blacklistSe(std::string name, std::string vo, std::string status, int timeout, bool mode);
    std::vector< std::pair<std::string, std::string>  > cancel(std::vector<std::string> const & jobIds);
    boost::tuple<int, int> cancelAll(const std::string& vo);
    void debugSet(std::string source, std::string destination, unsigned level);
    void delConfiguration(std::vector<std::string> const &cfgs);
    void delegate(std::string const & delegationId, long expirationTime);
    std::string deleteFile (const std::vector<std::string>& filesForDelete);
    void doDrain(bool drain);
    std::string getBandwidthLimit();
    std::vector<std::string> getConfiguration (std::string src, std::string dest, std::string all, std::string name);
    std::vector<DetailedFileStatus> getDetailedJobStatus(std::string const & jobId);
    std::vector<FileInfo> getFileStatus (std::string const & jobId, bool archive, int offset, int limit, bool retries);
    std::vector<Snapshot> getSnapShot(std::string const & vo, std::string const & src, std::string const & dst);
    JobStatus getTransferJobStatus (std::string const & jobId, bool archive);
    JobStatus getTransferJobSummary (std::string const & jobId, bool archive);
    long isCertValid();
    std::vector<JobStatus> listRequests (std::vector<std::string> const & statuses, std::string const & dn, std::string const & vo, std::string const & source, std::string const & destination);
    std::vector<JobStatus> listDeletionRequests (std::vector<std::string> const & statuses, std::string const & dn, std::string const & vo, std::string const & source, std::string const & destination);
    void optimizerModeSet(int mode);
    void prioritySet(std::string jobId, int priority);
    void queueTimeoutSet(unsigned timeout);
    void retrySet(std::string vo, int retry);
    void revoke(const std::string& op, const std::string& dn);
    void setBandwidthLimit(const std::string& source_se, const std::string& dest_se, int limit);
    void setConfiguration (std::vector<std::string> const &cfgs);
    void setDropboxCredential(std::string const & appKey, std::string const & appSecret, std::string const & apiUrl);
    void setFixActivePerPair(std::string source, std::string destination, int active);
    void setGlobalLimits(boost::optional<int> maxActivePerLink, boost::optional<int> maxActivePerSe);
    void setGlobalTimeout(int timeout);
    void setMaxDstSeActive(std::string se, int active);
    void setMaxOpt(std::tuple<std::string, int, std::string> const &, std::string const&);
    void setMaxSrcSeActive(std::string se, int active);
    void setS3Credential(std::string const & accessKey, std::string const & secretKey, std::string const & vo, std::string const & storage);
    void setSecPerMb(int secPerMb);
    void setSeProtocol(std::string protocol, std::string se, std::string state);
    void showUserDn(bool show);
    std::string transferSubmit (std::vector<File> const & files, std::map<std::string, std::string> const & parameters);


protected:

    std::string const capath;
    std::string const proxy;
    std::unique_ptr<ServiceAdapter> proxysvc;
    static volatile sig_atomic_t warngiven1;
    static volatile sig_atomic_t warngiven2;

    enum FallbackState { RESTWITHFB, REST, GSOAP } fbstate;

    void getInterfaceDetails();
    void initfacade();
    bool tryfallback(cli_exception const &ex);
};

} /* namespace cli */
} /* namespace fts3 */

#endif /* SERVICEADAPTERFALLBACKFACADE_H_ */

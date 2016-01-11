/*
 * ServiceAdapterFallbackFacade.cpp
 *
 *  Created on: 14 Oct 2015
 *      Author: dhsmith
 */

#include "ServiceAdapterFallbackFacade.h"

#include "GSoapContextAdapter.h"
#include "RestContextAdapter.h"

#include "MsgPrinter.h"

#include "exception/cli_exception.h"
#include "exception/wrong_protocol.h"

#include <boost/regex.hpp>
#include <cstdlib>
#include <signal.h>

namespace fts3
{
namespace cli
{

// track if we've warned about using gsoap or fallingback to gsoap,
// to limit warning messages if used many times in a program
volatile sig_atomic_t ServiceAdapterFallbackFacade::warngiven1 = 0;
volatile sig_atomic_t ServiceAdapterFallbackFacade::warngiven2 = 0;

void ServiceAdapterFallbackFacade::authorize(const std::string& op, const std::string& dn)
{
    initfacade();
    while(1) {
        try {
            proxysvc->authorize(op, dn);
            return;
        } catch(cli_exception const &ex) {
            if (!tryfallback(ex)) throw;
        }
    }
}

void ServiceAdapterFallbackFacade::blacklistDn(std::string subject, std::string status, int timeout, bool mode)
{
    initfacade();
    while(1) {
        try {
            proxysvc->blacklistDn(subject, status, timeout, mode);
            return;
        } catch(cli_exception const &ex) {
            if (!tryfallback(ex)) throw;
        }
    }
}

void ServiceAdapterFallbackFacade::blacklistSe(std::string name, std::string vo, std::string status, int timeout, bool mode)
{
    initfacade();
    while(1) {
        try {
            proxysvc->blacklistSe(name, vo, status, timeout, mode);
            return;
        } catch(cli_exception const &ex) {
            if (!tryfallback(ex)) throw;
        }
    }
}

std::vector< std::pair<std::string, std::string> > ServiceAdapterFallbackFacade::cancel(std::vector<std::string> const & jobIds)
{
    initfacade();
    while(1) {
        try {
            return proxysvc->cancel(jobIds);
        } catch(cli_exception const &ex) {
            if (!tryfallback(ex)) throw;
        }
    }
}

boost::tuple<int, int>  ServiceAdapterFallbackFacade::cancelAll(const std::string& vo)
{
    initfacade();
    while(1) {
        try {
            return proxysvc->cancelAll(vo);
        } catch(cli_exception const &ex) {
            if (!tryfallback(ex)) throw;
        }
    }
}

void ServiceAdapterFallbackFacade::debugSet(std::string source, std::string destination, unsigned level)
{
    initfacade();
    while(1) {
        try {
            proxysvc->debugSet(source, destination, level);
            return;
        } catch(cli_exception const &ex) {
            if (!tryfallback(ex)) throw;
        }
    }
}

void ServiceAdapterFallbackFacade::delConfiguration(std::vector<std::string> const &cfgs)
{
    initfacade();
    while(1) {
        try {
            proxysvc->delConfiguration(cfgs);
            return;
        } catch(cli_exception const &ex) {
            if (!tryfallback(ex)) throw;
        }
    }
}

void ServiceAdapterFallbackFacade::delegate(std::string const & delegationId, long expirationTime)
{
    initfacade();
    while(1) {
        try {
            proxysvc->delegate(delegationId, expirationTime);
            return;
        } catch(cli_exception const &ex) {
            if (!tryfallback(ex)) throw;
        }
    }
}

std::string ServiceAdapterFallbackFacade::deleteFile (const std::vector<std::string>& filesForDelete)
{
    initfacade();
    while(1) {
        try {
            return proxysvc->deleteFile(filesForDelete);
        } catch(cli_exception const &ex) {
            if (!tryfallback(ex)) throw;
        }
    }
}

void ServiceAdapterFallbackFacade::doDrain(bool drain)
{
    initfacade();
    while(1) {
        try {
            proxysvc->doDrain(drain);
            return;
        } catch(cli_exception const &ex) {
            if (!tryfallback(ex)) throw;
        }
    }
}

std::vector<std::string> ServiceAdapterFallbackFacade::getConfiguration(std::string src, std::string dest, std::string all, std::string name)
{
    initfacade();
    while(1) {
        try {
            return proxysvc->getConfiguration(src,dest,all,name);
        } catch(cli_exception const &ex) {
            if (!tryfallback(ex)) throw;
        }
    }
}

std::string ServiceAdapterFallbackFacade::getBandwidthLimit()
{
    initfacade();
    while(1) {
        try {
            return proxysvc->getBandwidthLimit();
        } catch(cli_exception const &ex) {
            if (!tryfallback(ex)) throw;
        }
    }
}

std::vector<DetailedFileStatus> ServiceAdapterFallbackFacade::getDetailedJobStatus(std::string const & jobId)
{
    initfacade();
    while(1) {
        try {
            return proxysvc->getDetailedJobStatus(jobId);
        } catch(cli_exception const &ex) {
            if (!tryfallback(ex)) throw;
        }
    }
}

std::vector<FileInfo> ServiceAdapterFallbackFacade::getFileStatus (std::string const & jobId, bool archive, int offset, int limit, bool retries)
{
    initfacade();
    while(1) {
        try {
            return proxysvc->getFileStatus(jobId, archive, offset, limit, retries);
        } catch(cli_exception const &ex) {
            if (!tryfallback(ex)) throw;
        }
    }
}

void ServiceAdapterFallbackFacade::getInterfaceDetails()
{
    initfacade();
    while(1) {
        try {
            proxysvc->getInterfaceDetails();
            interface = proxysvc->interface;
            version = proxysvc->version;
            schema = proxysvc->schema;
            metadata = proxysvc->metadata;
            return;
        } catch(cli_exception const &ex) {
            if (!tryfallback(ex)) throw;
        }
    }
}

std::vector<Snapshot> ServiceAdapterFallbackFacade::getSnapShot(std::string const & vo, std::string const & src, std::string const & dst)
{
    initfacade();
    while(1) {
        try {
            return proxysvc->getSnapShot(vo, src, dst);
        } catch(cli_exception const &ex) {
            if (!tryfallback(ex)) throw;
        }
    }
}

JobStatus ServiceAdapterFallbackFacade::getTransferJobStatus (std::string const & jobId, bool archive)
{
    initfacade();
    while(1) {
        try {
            return proxysvc->getTransferJobStatus(jobId, archive);
        } catch(cli_exception const &ex) {
            if (!tryfallback(ex)) throw;
        }
    }
}

JobStatus ServiceAdapterFallbackFacade::getTransferJobSummary (std::string const & jobId, bool archive)
{
    initfacade();
    while(1) {
        try {
            return proxysvc->getTransferJobSummary(jobId, archive);
        } catch(cli_exception const &ex) {
            if (!tryfallback(ex)) throw;
        }
    }
}

long ServiceAdapterFallbackFacade::isCertValid()
{
    initfacade();
    while(1) {
        try {
            return proxysvc->isCertValid();
        } catch(cli_exception const &ex) {
            if (!tryfallback(ex)) throw;
        }
    }
}

std::vector<JobStatus> ServiceAdapterFallbackFacade::listRequests (std::vector<std::string> const & statuses, std::string const & dn, std::string const & vo, std::string const &source, std::string const &destination)
{
    initfacade();
    while(1) {
        try {
            return proxysvc->listRequests(statuses, dn, vo, source, destination);
        } catch(cli_exception const &ex) {
            if (!tryfallback(ex)) throw;
        }
    }
}

std::vector<JobStatus> ServiceAdapterFallbackFacade::listDeletionRequests (std::vector<std::string> const & statuses, std::string const & dn, std::string const & vo, std::string const & source, std::string const & destination)
{
    initfacade();
    while(1) {
        try {
            return proxysvc->listDeletionRequests(statuses, dn, vo, source, destination);
        } catch(cli_exception const &ex) {
            if (!tryfallback(ex)) throw;
        }
    }
}


void ServiceAdapterFallbackFacade::optimizerModeSet(int mode)
{
    initfacade();
    while(1) {
        try {
            proxysvc->optimizerModeSet(mode);
            return;
        } catch(cli_exception const &ex) {
            if (!tryfallback(ex)) throw;
        }
    }
}

void ServiceAdapterFallbackFacade::prioritySet(std::string jobId, int priority)
{
    initfacade();
    while(1) {
        try {
            proxysvc->prioritySet(jobId, priority);
            return;
        } catch(cli_exception const &ex) {
            if (!tryfallback(ex)) throw;
        }
    }
}

void ServiceAdapterFallbackFacade::queueTimeoutSet(unsigned timeout)
{
    initfacade();
    while(1) {
        try {
            proxysvc->queueTimeoutSet(timeout);
            return;
        } catch(cli_exception const &ex) {
            if (!tryfallback(ex)) throw;
        }
    }
}

void ServiceAdapterFallbackFacade::retrySet(std::string vo, int retry)
{
    initfacade();
    while(1) {
        try {
            proxysvc->retrySet(vo, retry);
            return;
        } catch(cli_exception const &ex) {
            if (!tryfallback(ex)) throw;
        }
    }
}

void ServiceAdapterFallbackFacade::revoke(const std::string& op, const std::string& dn)
{
    initfacade();
    while(1) {
        try {
            proxysvc->revoke(op, dn);
            return;
        } catch(cli_exception const &ex) {
            if (!tryfallback(ex)) throw;
        }
    }
}

void ServiceAdapterFallbackFacade::setBandwidthLimit(const std::string& source_se, const std::string& dest_se, int limit)
{
    initfacade();
    while(1) {
        try {
            proxysvc->setBandwidthLimit(source_se, dest_se, limit);
            return;
        } catch(cli_exception const &ex) {
            if (!tryfallback(ex)) throw;
        }
    }
}

void ServiceAdapterFallbackFacade::setConfiguration(std::vector<std::string> const &cfgs)
{
    initfacade();
    while(1) {
        try {
            proxysvc->setConfiguration(cfgs);
            return;
        } catch(cli_exception const &ex) {
            if (!tryfallback(ex)) throw;
        }
    }
}

void ServiceAdapterFallbackFacade::setDropboxCredential(std::string const & appKey, std::string const & appSecret, std::string const & apiUrl)
{
    initfacade();
    while(1) {
        try {
            proxysvc->setDropboxCredential(appKey, appSecret, apiUrl);
            return;
        } catch(cli_exception const &ex) {
            if (!tryfallback(ex)) throw;
        }
    }
}

void ServiceAdapterFallbackFacade::setFixActivePerPair(std::string source, std::string destination, int active)
{
    initfacade();
    while(1) {
        try {
            proxysvc->setFixActivePerPair(source, destination, active);
            return;
        } catch(cli_exception const &ex) {
            if (!tryfallback(ex)) throw;
        }
    }
}

void ServiceAdapterFallbackFacade::setGlobalLimits(boost::optional<int> maxActivePerLink, boost::optional<int> maxActivePerSe)
{
    initfacade();
    while(1) {
        try {
            proxysvc->setGlobalLimits(maxActivePerLink, maxActivePerSe);
            return;
        } catch(cli_exception const &ex) {
            if (!tryfallback(ex)) throw;
        }
    }
}

void ServiceAdapterFallbackFacade::setGlobalTimeout(int timeout)
{
    initfacade();
    while(1) {
        try {
            proxysvc->setGlobalTimeout(timeout);
            return;
        } catch(cli_exception const &ex) {
            if (!tryfallback(ex)) throw;
        }
    }
}

void ServiceAdapterFallbackFacade::setMaxDstSeActive(std::string se, int active)
{
    initfacade();
    while(1) {
        try {
            proxysvc->setMaxDstSeActive(se, active);
            return;
        } catch(cli_exception const &ex) {
            if (!tryfallback(ex)) throw;
        }
    }
}

void ServiceAdapterFallbackFacade::setMaxOpt(std::tuple<std::string, int, std::string> const &triplet, std::string const &opt)
{
    initfacade();
    while(1) {
        try {
            proxysvc->setMaxOpt(triplet, opt);
            return;
        } catch(cli_exception const &ex) {
            if (!tryfallback(ex)) throw;
        }
    }
}

void ServiceAdapterFallbackFacade::setMaxSrcSeActive(std::string se, int active)
{
    initfacade();
    while(1) {
        try {
            proxysvc->setMaxSrcSeActive(se, active);
            return;
        } catch(cli_exception const &ex) {
            if (!tryfallback(ex)) throw;
        }
    }
}

void ServiceAdapterFallbackFacade::setS3Credential(std::string const & accessKey, std::string const & secretKey, std::string const & vo, std::string const & storage)
{
    initfacade();
    while(1) {
        try {
            proxysvc->setS3Credential(accessKey, secretKey, vo, storage);
            return;
        } catch(cli_exception const &ex) {
            if (!tryfallback(ex)) throw;
        }
    }
}

void ServiceAdapterFallbackFacade::setSecPerMb(int secPerMb)
{
    initfacade();
    while(1) {
        try {
            proxysvc->setSecPerMb(secPerMb);
            return;
        } catch(cli_exception const &ex) {
            if (!tryfallback(ex)) throw;
        }
    }
}

void ServiceAdapterFallbackFacade::setSeProtocol(std::string protocol, std::string se, std::string state)
{
    initfacade();
    while(1) {
        try {
            proxysvc->setSeProtocol(protocol, se, state);
            return;
        } catch(cli_exception const &ex) {
            if (!tryfallback(ex)) throw;
        }
    }
}

void ServiceAdapterFallbackFacade::showUserDn(bool show)
{
    initfacade();
    while(1) {
        try {
            proxysvc->showUserDn(show);
            return;
        } catch(cli_exception const &ex) {
            if (!tryfallback(ex)) throw;
        }
    }
}

std::string ServiceAdapterFallbackFacade::transferSubmit (std::vector<File> const & files, std::map<std::string, std::string> const & parameters)
{
    initfacade();
    while(1) {
        try {
            return proxysvc->transferSubmit(files, parameters);
        } catch(cli_exception const &ex) {
            if (!tryfallback(ex)) throw;
        }
    }
}

// *****************************************************************************

ServiceAdapterFallbackFacade::ServiceAdapterFallbackFacade(const ServiceAdapterFallbackFacade &other) :
    ServiceAdapter(other), capath(other.capath), proxy(other.proxy)
{
    fbstate = other.fbstate;
    if (!other.proxysvc) {
        return;
    }
    if (fbstate == RESTWITHFB || fbstate == REST) {
        proxysvc.reset(new RestContextAdapter(endpoint, capath, proxy));
    } else {
        proxysvc.reset(new GSoapContextAdapter(endpoint, proxy));
    }
}

void ServiceAdapterFallbackFacade::initfacade()
{
    if (proxysvc) {
        // do nothing more
        return;
    }

    // setup the fallback state
    fbstate = RESTWITHFB;

    // from http://tools.ietf.org/html/rfc3986#appendix-B
    boost::regex url_decompose_regex("^(([^:/?#]+):)?(//([^/?#]*))?([^?#]*)(\\?([^#]*))?(#(.*))?$");

    boost::regex authority_port(".*:([^:@\\[\\]]*?)$");
    boost::cmatch what,what2;

    // check which port is specified
    if (boost::regex_match(endpoint.c_str(), what, url_decompose_regex)) {
        std::string authority(what[4].first, what[4].second);
        if (boost::regex_match(authority.c_str(), what2, authority_port)) {
            std::string portstr(what2[1].first, what2[1].second);
            int port = std::atoi(portstr.c_str());
            if (port == 8443) {
                if (!warngiven1) {
                    // race here may lead to message being given more than once.
                    // more complete exclusion thought not necessary.
                    warngiven1 = 1;
                    std::cerr << "warning : fts client is connecting using the "
                        "gSOAP interface. Consider changing" << std::endl <<
                        "          your configured fts endpoint port to select "
                        "the REST interface." << std::endl;
                }
                fbstate = GSOAP;
            } else if (port == 8446) {
                fbstate = REST;
            }
        }
    }

    // setup the service to be proxied initially
    if (fbstate == RESTWITHFB || fbstate == REST) {
        proxysvc.reset(new RestContextAdapter(endpoint, capath, proxy));
    } else {
        proxysvc.reset(new GSoapContextAdapter(endpoint, proxy));
    }
}

bool ServiceAdapterFallbackFacade::tryfallback(cli_exception const &ex)
{
    if (fbstate != RESTWITHFB) {
        return false;
    }

    if (!ex.tryFallback()) {
        return false;
    }

    fbstate = GSOAP;
    proxysvc.reset(new GSoapContextAdapter(endpoint, proxy));
    interface.clear();
    version.clear();
    schema.clear();
    metadata.clear();
    if (!warngiven2) {
        // race here may lead to message being given more than once.
        // more complete exclusion thought not necessary.
        warngiven2 = 1;
        std::cerr << "warning : " << std::string(ex.what()) << ". Going to"
            << std::endl << "          try again using gSOAP to communicate "
            "with the fts endpoint." << std::endl;
    }

    return true;
}

} /* namespace cli */
} /* namespace fts3 */

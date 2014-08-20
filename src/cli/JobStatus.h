/*
 * JobStatus.h
 *
 *  Created on: 11 Aug 2014
 *      Author: simonm
 */

#ifndef JOBSTATUS_H_
#define JOBSTATUS_H_

#include "ws-ifce/gsoap/gsoap_stubs.h"

#include <tuple>
#include <string>
#include <vector>
#include <iterator>
#include <algorithm>

#include <boost/optional.hpp>
#include <boost/bind/bind.hpp>

#include <boost/property_tree/json_parser.hpp>

namespace fts3 {
namespace cli {

namespace pt = boost::property_tree;

class JobStatus
{
    friend class MsgPrinter;
    friend class JsonOutput;

    struct FileInfo
    {
        std::string src;
        std::string dst;
        std::string state;
        std::string reason;
        long duration;
        int nbFailures;
        std::vector<std::string> retries;
    };

public:

    typedef std::tuple<int, int, int, int, int , int> JobSummary;

    JobStatus(std::string const & jobId, std::string const & status, std::string const & dn, std::string const & reason,
              std::string const & vo, std::string const & submitTime, int nbFiles, int priority,
              boost::optional<JobSummary> summary = boost::optional<JobSummary>()) :
            jobId(jobId),
            status(status),
            dn(dn),
            reason(reason),
            vo (vo),
            submitTime(submitTime),
            nbFiles(nbFiles),
            priority(priority),
            summary(summary) {}

    virtual ~JobStatus() {}

    void addFile(tns3__FileTransferStatus const & gsoapFile)
    {
        FileInfo file;
        file.src = *gsoapFile.sourceSURL;
        file.dst = *gsoapFile.destSURL;
        file.state = *gsoapFile.transferFileState;
        file.nbFailures = gsoapFile.numFailures;
        file.reason = *gsoapFile.reason;
        file.duration = gsoapFile.duration;

        std::transform(
            gsoapFile.retries.begin(),
            gsoapFile.retries.end(),
            std::inserter(file.retries, file.retries.begin()),
            boost::bind(&tns3__FileTransferRetry::reason, _1)
        );

        files.push_back(file);
    }

    std::string getStatus()
    {
        return status;
    }

private:

    std::string jobId;
    std::string status;
    std::string dn;
    std::string reason;
    std::string vo;
    std::string submitTime;
    int nbFiles;
    int priority;

    boost::optional<JobSummary> summary;

    std::vector<FileInfo> files;
};



} /* namespace cli */
} /* namespace fts3 */

#endif /* JOBSTATUS_H_ */

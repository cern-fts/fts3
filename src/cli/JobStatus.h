/*
 * JobStatus.h
 *
 *  Created on: 11 Aug 2014
 *      Author: simonm
 */

#ifndef JOBSTATUS_H_
#define JOBSTATUS_H_

#include "ws-ifce/gsoap/gsoap_stubs.h"

#include "exception/cli_exception.h"

#include <time.h>

#include <tuple>
#include <string>
#include <vector>
#include <iterator>
#include <algorithm>

#include <boost/optional.hpp>
#include <boost/bind/bind.hpp>

#include <boost/property_tree/json_parser.hpp>

namespace fts3
{
namespace cli
{

namespace pt = boost::property_tree;

class FileInfo
{
    friend class MsgPrinter;
    friend class JsonOutput;

public:

    FileInfo(tns3__FileTransferStatus const * f) :
        src(*f->sourceSURL), dst (*f->destSURL), fileIdAvail(false), state(*f->transferFileState),
        reason(*f->reason), duration (f->duration), nbFailures(f->numFailures),
        staging_duration(-1)
    {
        std::transform(
            f->retries.begin(),
            f->retries.end(),
            std::back_inserter(retries),
            boost::bind(&tns3__FileTransferRetry::reason, _1)
        );

        if (f->staging)
            staging_duration = *f->staging;
    }

    FileInfo(pt::ptree const & t) :
        src(t.get<std::string>("source_surl")), dst(t.get<std::string>("dest_surl")), fileId(t.get<int>("file_id")),
        fileIdAvail(true), state(t.get<std::string>("file_state")), reason(t.get<std::string>("reason")), duration(0),
        nbFailures(t.get<int>("retry")), staging_duration(0)
    {
        try {
            pt::ptree const & r = t.get_child("retries");
            setRetries(r);
        } catch(const pt::ptree_bad_path &) {
            // retries path may not be available
        }

        std::string finish_time = t.get<std::string>("finish_time");
        std::string start_time = t.get<std::string>("start_time");

        tm time;
        memset(&time, 0, sizeof(time));

        strptime(finish_time.c_str(), "%Y-%m-%dT%H:%M:%S", &time);
        time_t finish = timegm(&time);

        strptime(start_time.c_str(), "%Y-%m-%dT%H:%M:%S", &time);
        time_t start = timegm(&time);

        duration = (long)difftime(finish, start);

        std::string staging_start = t.get<std::string>("staging_start");
        std::string staging_finished = t.get<std::string>("staging_finished");

        if (strptime(staging_start.c_str(), "%Y-%m-%dT%H:%M:%S", &time) != NULL)
            {
                time_t staging_start_time = timegm(&time);
                time_t staging_finished_time = ::time(NULL);

                if (strptime(staging_finished.c_str(), "%Y-%m-%dT%H:%M:%S", &time) != NULL)
                    staging_finished_time = timegm(&time);

                staging_duration = staging_finished_time - staging_start_time;
            }
    }

    void setRetries(pt::ptree const & r)
    {
        pt::ptree::const_iterator itr;

        retries.clear();
        for (itr = r.begin(); itr != r.end(); ++itr)
            {
                std::string reason(itr->second.get<std::string>("reason"));
                retries.push_back(reason);
            }
    }

    std::string getState() const
    {
        return state;
    }

    std::string getSource() const
    {
        return src;
    }

    std::string getDestination() const
    {
        return dst;
    }

    int getFileId() const
    {
        if (!fileIdAvail) {
            throw cli_exception("The file id is not available");
        }
        return fileId;
    }

private:
    std::string src;
    std::string dst;
    int fileId;
    bool fileIdAvail;
    std::string state;
    std::string reason;
    long duration;
    int nbFailures;
    std::vector<std::string> retries;
    long staging_duration;
};

class DetailedFileStatus
{
    friend class MsgPrinter;
    friend class JsonOutput;

public:
    DetailedFileStatus(tns3__DetailedFileStatus const * df) :
        jobId(df->jobId), src(df->sourceSurl), dst(df->destSurl), fileId(df->fileId), state(df->fileState)
    {

    }

    DetailedFileStatus(pt::ptree const & t) :
        jobId(t.get<std::string>("job_id")), src(t.get<std::string>("source_surl")), dst(t.get<std::string>("dest_surl")),
        fileId(t.get<int>("file_id")), state(t.get<std::string>("file_state"))
    {

    }

private:
    std::string jobId;
    std::string src;
    std::string dst;
    int fileId;
    std::string state;
};

class JobStatus
{
    friend class MsgPrinter;
    friend class JsonOutput;

public:

    typedef std::tuple<int, int, int, int, int , int, int, int, int> JobSummary;

    JobStatus(std::string const & jobId, std::string const & status, std::string const & dn, std::string const & reason,
              std::string const & vo, std::string const & submitTime, int nbFiles, int priority,
              boost::optional<JobSummary> summary = boost::optional<JobSummary>()) :
        jobId(jobId), status(status), dn(dn), reason(reason), vo (vo), submitTime(submitTime),
        nbFiles(nbFiles), priority(priority), summary(summary)
    {

    }

    virtual ~JobStatus() {}

    void addFile(FileInfo const & file)
    {
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

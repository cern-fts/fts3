/*
 *	Copyright notice:
 *	Copyright © Members of the EMI Collaboration, 2010.
 *
 *	See www.eu-emi.eu for details on the copyright holders
 *
 *	Licensed under the Apache License, Version 2.0 (the "License");
 *	you may not use this file except in compliance with the License.
 *	You may obtain a copy of the License at
 *
 *		http://www.apache.org/licenses/LICENSE-2.0
 *
 *	Unless required by applicable law or agreed to in writing, software
 *	distributed under the License is distributed on an "AS IS" BASIS,
 *	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or impltnsied.
 *	See the License for the specific language governing permissions and
 *	limitations under the License.
 *
 * JobSubmitter.cpp
 *
 *  Created on: Mar 7, 2012
 *      Author: Michal Simon
 */

#include "JobSubmitter.h"

#include "TransferCreator.h"
#include "PlainOldJob.h"

#include "common/uuid_generator.h"
#include "db/generic/SingleDbInstance.h"

#include "common/logger.h"
#include "common/error.h"

#include "config/serverconfig.h"

#include "ws/CGsiAdapter.h"
#include "ws/delegation/GSoapDelegationHandler.h"
#include "ws/config/Configuration.h"

#include "profiler/Macros.h"

#include <boost/lexical_cast.hpp>

#include <algorithm>
#include <numeric>

#include <boost/lambda/lambda.hpp>
#include <boost/optional.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/assign.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/tuple/tuple.hpp>

#include "parse_url.h"

#include "ws/SingleTrStateInstance.h"

using namespace db;
using namespace fts3::config;
using namespace fts3::ws;
using namespace boost;
using namespace boost::assign;


const regex JobSubmitter::fileUrlRegex("([a-zA-Z][a-zA-Z0-9+\.-]*://[a-zA-Z0-9\\.-]+)(:\\d+)?/.+");

const string JobSubmitter::srm_protocol = "srm";

static void checkValidUrl(const std::string &uri)
{
    Uri u0 = Uri::Parse(uri);
    bool ok = u0.Host.length() != 0 && u0.Protocol.length() != 0 && u0.Path.length() != 0;
    if (!ok)
        {
            std::string errMsg = "Not valid uri format, check submitted uri's";
            throw Err_Custom(errMsg);
        }
}

JobSubmitter::JobSubmitter(soap* ctx, tns3__TransferJob *job, bool delegation) :
    db (DBSingleton::instance().getDBObjectInstance()),
    copyPinLifeTime(-1),
    srm_source(true)
{
    PROFILE_SCOPE("JobSubmitter::JobSubmitter(soap*, tns3__TransferJob*, bool)");

    // check the delegation and MyProxy password settings
    if (delegation)
        {
            if (job->credential)
                {
                    throw Err_Custom("The MyProxy password should not be provided if delegation is used");
                }
        }
    else
        {
            if (params.isParamSet(JobParameterHandler::DELEGATIONID))
                {
                    throw Err_Custom("The delegation ID should not be provided if MyProxy password mode is used");
                }

            if (!job->credential || job->credential->empty())
                {
                    throw Err_Custom("The MyProxy password is empty while submitting in MyProxy mode");
                }

            cred = *job->credential;
        }

    // do the common initialisation
    init(ctx, job);
    // it is a plain old job
    PlainOldJob<tns3__TransferJobElement> poj(job->transferJobElements, initialState);
    // extract the job elements from tns3__TransferJob2 object and put them into a vector
    poj.get(jobs, vo);
    // set additional info
    srm_source = poj.isSrm();
    sourceSe = poj.getSourceSe();
    destinationSe = poj.getDestinationSe();
}

JobSubmitter::JobSubmitter(soap* ctx, tns3__TransferJob2 *job) :
    db (DBSingleton::instance().getDBObjectInstance()),
    copyPinLifeTime(-1),
    srm_source(true)
{
    PROFILE_SCOPE("JobSubmitter::JobSubmitter(soap*, tns3__TransferJob2*)");

    // checksum uses always delegation?
    if (job->credential)
        {
            throw Err_Custom("The MyProxy password should not be provided if delegation is used");
        }

    // do the common initialisation
    init(ctx, job);

    // it is a plain old job
    PlainOldJob<tns3__TransferJobElement2> poj(job->transferJobElements, initialState);
    // extract the job elements from tns3__TransferJob2 object and put them into a vector
    poj.get(jobs, vo, params);
    // set additional info
    srm_source = poj.isSrm();
    sourceSe = poj.getSourceSe();
    destinationSe = poj.getDestinationSe();
}

JobSubmitter::JobSubmitter(soap* ctx, tns3__TransferJob3 *job) :
    db (DBSingleton::instance().getDBObjectInstance()),
    copyPinLifeTime(-1)
{
    PROFILE_SCOPE("JobSubmitter::JobSubmitter(soap*, tns3__TransferJob3*)");

    // do the common initialisation
    init(ctx, job);

    // index of the file
    int fileIndex = 0;

    // the object in charge of blacklist compatibility
    BlacklistInspector inspector(vo);

    // if at least one source uses different protocol than SRM it will be 'false'
    srm_source = true;

    // extract the job elements from tns3__TransferJob2 object and put them into a vector
    vector<tns3__TransferJobElement3 * >::iterator it;
    for (it = job->transferJobElements.begin(); it < job->transferJobElements.end(); it++)
        {
            tns3__TransferJobElement3* elem = (*it);

            // prepare the job element and add it to the job
            job_element_tupple tupple;

            // common properties
            tupple.filesize = elem->filesize ? *elem->filesize : 0;
            tupple.metadata = elem->metadata ? *elem->metadata : string();

            tupple.selectionStrategy = elem->selectionStrategy ? *elem->selectionStrategy : string();
            if (!tupple.selectionStrategy.empty() && tupple.selectionStrategy != "orderly" && tupple.selectionStrategy != "auto")
                throw Err_Custom("'" + tupple.selectionStrategy + "'");

            // in the future the checksum should be assigned to pairs!
            if (!elem->checksum.empty())
                {
                    tupple.checksum = (*it)->checksum.front();
                    if (!params.isParamSet(JobParameterHandler::CHECKSUM_METHOD))
                        params.set(JobParameterHandler::CHECKSUM_METHOD, "relaxed");
                }

            // pair sources with destinations and assign status

            TransferCreator tc(fileIndex, initialState);
            std::list< boost::tuple<std::string, std::string, std::string, int> > tuples =
            		tc.pairSourceAndDestination(elem->source, elem->dest);
            fileIndex = tc.nextFileIndex();

            // if it is not multiple source/destination submission ..
            if (tuples.size() == 1)
                {
                    // add the source and destination SE for the transfer job
                    sourceSe = fileUrlToSeName(boost::get<0>(tuples.front()), true);
                    destinationSe = fileUrlToSeName(boost::get<1>(tuples.front()));
                }

            // multiple pairs and reuse are not compatible!
            if (tuples.size() > 1 && params.get(JobParameterHandler::REUSE) == "Y")
                {
                    throw Err_Custom("Reuse and multiple replica selection are incompatible!");
                }

            // TODO support flat file multi source/destination transfer job

            // add each pair
            std::list< boost::tuple<std::string, std::string, std::string, int> >::iterator it_p;
            for (it_p = tuples.begin(); it_p != tuples.end(); it_p++)
                {
                    // set the values for source and destination
                    tupple.source = boost::get<0>(*it_p);
                    tupple.destination = boost::get<1>(*it_p);
                    tupple.state = boost::get<2>(*it_p);
                    tupple.fileIndex = boost::get<3>(*it_p);

                    std::string sourceSe = fileUrlToSeName(boost::get<0>(*it_p), true);
                    std::string destinationSe = fileUrlToSeName(boost::get<1>(*it_p));
                    inspector.add(sourceSe);
                    inspector.add(destinationSe);

                    // check if all the sources use SRM protocol
                    srm_source &= sourceSe.find(srm_protocol) == 0;

                    tupple.source_se = sourceSe;
                    tupple.dest_se = destinationSe;

                    jobs.push_back(tupple);
                }
        }

    inspector.inspect();
    inspector.setWaitTimeout(jobs);
}

template <typename JOB>
void JobSubmitter::init(soap* ctx, JOB* job)
{
    GSoapDelegationHandler handler (ctx);
    delegationId = handler.makeDelegationId();

    CGsiAdapter cgsi (ctx);
    vo = cgsi.getClientVo();
    dn = cgsi.getClientDn();

    FTS3_COMMON_LOGGER_NEWLOG (INFO) << "DN: " << dn << " is submitting a transfer job" << commit;

    if (db->isDnBlacklisted(dn))
        {
            throw Err_Custom("The DN: " + dn + " is blacklisted!");
        }

    // check weather the job is well specified
    if (job == 0 || job->transferJobElements.empty())
        {
            throw Err_Custom("The job was not defined");
        }

    id = UuidGenerator::generateUUID();
    FTS3_COMMON_LOGGER_NEWLOG (DEBUG) << "Generated uuid " << id << commit;

    if (job->jobParams)
        {
            params(job->jobParams->keys, job->jobParams->values);
            //FTS3_COMMON_LOGGER_NEWLOG (DEBUG) << "Parameter map has been created" << commit;
        }

    bool use_bring_online =
    		params.isParamSet(JobParameterHandler::BRING_ONLINE) &&
    		params.get<int>(JobParameterHandler::BRING_ONLINE) > 0 &&
    		params.isParamSet(JobParameterHandler::COPY_PIN_LIFETIME) &&
    		params.get<int>(JobParameterHandler::COPY_PIN_LIFETIME) > 0;

    initialState = (use_bring_online ? "STAGING" : "SUBMITTED");
}

string JobSubmitter::getActivity(string metadata)
{
    // default value returned if the metadata are empty or an activity was not specified
    static const string defstr = "default";
    // if metadata are empty return default
    if (metadata.empty()) return defstr;
    // regular expression for finding the activity value
    static const regex re("^.*\"activity\"\\s*:\\s*\"(.+)\".*$");
    // look for the activity in the string and if it's there return the value
    smatch what;
    if (regex_match(metadata, what, re, match_extra)) return what[1];
    // if the activity was not found return default
    return defstr;
}

JobSubmitter::~JobSubmitter()
{

}

string JobSubmitter::submit()
{

    // for backwards compatibility check if copy-pin-lifetime and bring-online were set properly
    if (!params.isParamSet(JobParameterHandler::COPY_PIN_LIFETIME))
        {
            params.set(JobParameterHandler::COPY_PIN_LIFETIME, "-1");
        }

    if (!params.isParamSet(JobParameterHandler::BRING_ONLINE))
        {
            params.set(JobParameterHandler::BRING_ONLINE, "-1");
        }
    else
        {
            // make sure that bring online has been used for SRM source
            // (bring online is not supported for multiple source/destination submission)
            if (params.get(JobParameterHandler::BRING_ONLINE) != "-1" && !srm_source)
                throw Err_Custom("The 'bring-online' operation can be used only with source SEs that are using SRM protocol!");
        }

    if (!params.isParamSet(JobParameterHandler::RETRY))
        {
            params.set(JobParameterHandler::RETRY, "0");
        }

    if (!params.isParamSet(JobParameterHandler::RETRY_DELAY))
        {
            params.set(JobParameterHandler::RETRY_DELAY, "0");
        }

    bool protocol =
        params.isParamSet(JobParameterHandler::TIMEOUT) ||
        params.isParamSet(JobParameterHandler::NOSTREAMS) ||
        params.isParamSet(JobParameterHandler::BUFFER_SIZE)
        ;

    // if at least one protocol parameter was set make sure they are all set
    // use the defaults to fill the gaps
    if (protocol)
        {
            if (!params.isParamSet(JobParameterHandler::TIMEOUT))
                {
                    params.set(JobParameterHandler::TIMEOUT, "3600");
                }

            if (!params.isParamSet(JobParameterHandler::NOSTREAMS))
                {
                    params.set(JobParameterHandler::NOSTREAMS, "4");
                }

            if (!params.isParamSet(JobParameterHandler::BUFFER_SIZE))
                {
                    params.set(JobParameterHandler::BUFFER_SIZE, "0");
                }
        }


    // submit the transfer job (add it to the DB)
    db->submitPhysical (
        id,
        jobs,
        dn,
        cred,
        vo,
        string(),
        delegationId,
        sourceSe,
        destinationSe,
        params
    );

    //send state message - disabled for now but pls do not remove it
    //SingleTrStateInstance::instance().sendStateMessage(id, -1);

    FTS3_COMMON_LOGGER_NEWLOG (INFO) << "The jobid " << id << " has been submitted successfully" << commit;
    return id;
}

void JobSubmitter::checkProtocol(string file, bool source)
{
    string tmp (file);
    transform(tmp.begin(), tmp.end(), tmp.begin(), ::tolower);
    trim(tmp);

    bool not_ok =
        // check protocol
        !(tmp.find("mock://") == 0 || tmp.find("root://") == 0 || tmp.find("srm://") == 0 || tmp.find("gsiftp://") == 0 ||
          tmp.find("https://") == 0 || tmp.find("lfc://") == 0 || tmp.find("davs://") == 0)
        &&
        // check if lfn if it is the source
        (!source || !(file.find("/") == 0 && file.find(";") == string::npos && file.find(":") == string::npos))
        ;

    if(not_ok)
        {
            string msg = (source ? "Source" : "Destination");
            msg += " protocol is not supported for file: "  + file;
            throw Err_Custom(msg);
        }
}

string JobSubmitter::fileUrlToSeName(string url, bool source)
{
    checkValidUrl(url);
    checkProtocol(url, source);

    smatch what;
    if (regex_match(url, what, fileUrlRegex, match_extra))
        {
            // indexes are shifted by 1 because at index 0 is the whole string
            return string(what[1]);

        }
    else
        {
            string errMsg = "Can't extract hostname from url: " + url;
            throw Err_Custom(errMsg);
        }
}


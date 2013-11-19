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
#include "ws/config/Configuration.h"

#include "common/uuid_generator.h"
#include "db/generic/SingleDbInstance.h"

#include "common/logger.h"
#include "common/error.h"

#include "config/serverconfig.h"

#include "ws/CGsiAdapter.h"
#include "ws/delegation/GSoapDelegationHandler.h"

#include "infosys/OsgParser.h"

#include "profiler/Macros.h"

#include <boost/lexical_cast.hpp>

#include <algorithm>

#include <boost/optional.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/assign.hpp>

#include "parse_url.h"

#include "ws/SingleTrStateInstance.h"

#include <time.h>
#include <iostream>

using namespace db;
using namespace config;
using namespace fts3::infosys;
using namespace fts3::ws;
using namespace boost;
using namespace boost::assign;


const regex JobSubmitter::fileUrlRegex("(.+://[a-zA-Z0-9\\.-]+)(:\\d+)?/.+");

const string JobSubmitter::false_str = "false";

const string JobSubmitter::srm_protocol = "srm";

static bool checkValidUrl(const std::string &uri)
{
    Uri u0 = Uri::Parse(uri);
    return  u0.Host.length() != 0 && u0.Protocol.length() != 0 && u0.Path.length() != 0;
}

JobSubmitter::JobSubmitter(soap* soap, tns3__TransferJob *job, bool delegation) :
    db (DBSingleton::instance().getDBObjectInstance())
{
    PROFILE_SCOPE("JobSubmitter::JobSubmitter(soap*, tns3__TransferJob*, bool)");

    GSoapDelegationHandler handler(soap);
    delegationId = handler.makeDelegationId();

    CGsiAdapter cgsi(soap);
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

    // do the common initialization
    init(job->jobParams);

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

    // if at least one source uses different protocol than SRM it will be 'false'
    srm_source = true;

    // index of the file
    int fileIndex = 0;

    // extract the job elements from tns3__TransferJob object and put them into a vector
    vector<tns3__TransferJobElement * >::iterator it;
    for (it = job->transferJobElements.begin(); it < job->transferJobElements.end(); ++it, ++fileIndex)
        {

            string src = *(*it)->source, dest = *(*it)->dest;

            if (!checkValidUrl(src) || !checkValidUrl(dest))
                {
                    std::string errMsg = "Not valid uri format, check submitted uri's";
                    throw Err_Custom(errMsg);
                }

            string sourceSe = fileUrlToSeName(src);
            if(sourceSe.empty())
                {
                    std::string errMsg = "Can't extract hostname from url " + src;
                    throw Err_Custom(errMsg);
                }
            checkSe(sourceSe, vo);
            // set the source SE for the transfer job,
            // in case of this submission type multiple
            // source/destination submission is not possible
            // so we can just pick the first one
            if (this->sourceSe.empty())
                {
                    this->sourceSe = sourceSe;
                }
            // check if all the sources use SRM protocol
            srm_source &= sourceSe.find(srm_protocol) == 0;

            string destinationSe = fileUrlToSeName(dest);
            if(destinationSe.empty())
                {
                    std::string errMsg = "Can't extract hostname from url " + dest;
                    throw Err_Custom(errMsg);
                }
            checkSe(destinationSe, vo);
            // set the destination SE for the transfer job,
            // in case of this submission type multiple
            // source/destination submission is not possible
            // so we can just pick the first one
            if (this->destinationSe.empty())
                {
                    this->destinationSe = destinationSe;
                }

            // check weather the source and destination files are supported
            if (!checkProtocol(dest)) throw Err_Custom("Destination protocol not supported (" + dest + ")");
            if (!checkProtocol(src) && !checkIfLfn(src)) throw Err_Custom("Source protocol not supported (" + src + ")");

            job_element_tupple tupple;
            tupple.source = src;
            tupple.destination = dest;
            tupple.source_se = sourceSe;
            tupple.dest_se = destinationSe;
            tupple.checksum = string();
            tupple.filesize = 0;
            tupple.metadata = string();
            tupple.fileIndex = fileIndex;

            jobs.push_back(tupple);
        }
    //FTS3_COMMON_LOGGER_NEWLOG (DEBUG) << "Job's vector has been created" << commit;
}

JobSubmitter::JobSubmitter(soap* soap, tns3__TransferJob2 *job) :
    db (DBSingleton::instance().getDBObjectInstance())
{
    PROFILE_SCOPE("JobSubmitter::JobSubmitter(soap*, tns3__TransferJob2*)");

    GSoapDelegationHandler handler (soap);
    delegationId = handler.makeDelegationId();

    CGsiAdapter cgsi (soap);
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

    // checksum uses always delegation?
    if (job->credential)
        {
            throw Err_Custom("The MyProxy password should not be provided if delegation is used");
        }

    // do the common initialization
    init(job->jobParams);

    // if at least one source uses different protocol than SRM it will be 'false'
    srm_source = true;

    // index of the file
    int fileIndex = 0;

    // extract the job elements from tns3__TransferJob2 object and put them into a vector
    vector<tns3__TransferJobElement2 * >::iterator it;
    for (it = job->transferJobElements.begin(); it < job->transferJobElements.end(); ++it, ++fileIndex)
        {

            string src = *(*it)->source, dest = *(*it)->dest;

            if (!checkValidUrl(src) || !checkValidUrl(dest))
                {
                    std::string errMsg = "Not valid uri format, check submitted uri's";
                    throw Err_Custom(errMsg);
                }

            string sourceSe = fileUrlToSeName(src);
            if(sourceSe.empty())
                {
                    std::string errMsg = "Can't extract hostname from url " + src;
                    throw Err_Custom(errMsg);
                }
            checkSe(sourceSe, vo);
            // set the source SE for the transfer job,
            // in case of this submission type multiple
            // source/destination submission is not possible
            // so we can just pick the first one
            if (this->sourceSe.empty())
                {
                    this->sourceSe = sourceSe;
                }

            // check if all the sources use SRM protocol
            srm_source &= sourceSe.find(srm_protocol) == 0;

            string destinationSe = fileUrlToSeName(dest);
            if(destinationSe.empty())
                {
                    std::string errMsg = "Can't extract hostname from url " + dest;
                    throw Err_Custom(errMsg);
                }
            checkSe(destinationSe, vo);
            // set the destination SE for the transfer job,
            // in case of this submission type multiple
            // source/destination submission is not possible
            // so we can just pick the first one
            if (this->destinationSe.empty())
                {
                    this->destinationSe = destinationSe;
                }

            // check weather the destination file is supported
            if (!checkProtocol(dest))
                {
                    throw Err_Custom("Destination protocol is not supported for file: " + dest);
                }
            // check weather the source file is supported
            if (!checkProtocol(src) && !checkIfLfn(src))
                {
                    throw Err_Custom("Source protocol is not supported for file: " + src);
                }
            job_element_tupple tupple;
            tupple.source = src;
            tupple.destination = dest;
            tupple.source_se = sourceSe;
            tupple.dest_se = destinationSe;
            if((*it)->checksum)
                {
                    tupple.checksum = *(*it)->checksum;
                    if (!params.isParamSet(JobParameterHandler::CHECKSUM_METHOD))
                        params.set(JobParameterHandler::CHECKSUM_METHOD, "relaxed");
                }
            tupple.filesize = 0;
            tupple.metadata = string();
            tupple.fileIndex = fileIndex;

            jobs.push_back(tupple);
        }
    //FTS3_COMMON_LOGGER_NEWLOG (DEBUG) << "Job's vector has been created" << commit;
}

JobSubmitter::JobSubmitter(soap* ctx, tns3__TransferJob3 *job) :
    db (DBSingleton::instance().getDBObjectInstance())
{
    PROFILE_SCOPE("JobSubmitter::JobSubmitter(soap*, tns3__TransferJob3*)");

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

    // do the common initialization
    init(job->jobParams);

    // index of the file
    int fileIndex = 0;

    // if at least one source uses different protocol than SRM it will be 'false'
    srm_source = true;

//    jobs.reserve(job->transferJobElements.size());

    // extract the job elements from tns3__TransferJob2 object and put them into a vector
    vector<tns3__TransferJobElement3 * >::iterator it;
    for (it = job->transferJobElements.begin(); it < job->transferJobElements.end(); it++, fileIndex++)
        {

            // prepare the job element and add it to the job
            job_element_tupple tupple;

            // common properties
            tupple.filesize = (*it)->filesize ? *(*it)->filesize : 0;
            tupple.metadata = (*it)->metadata ? *(*it)->metadata : string();
            tupple.selectionStrategy = (*it)->selectionStrategy ? *(*it)->selectionStrategy : string();
            tupple.fileIndex = fileIndex;

            // TODO for now we just use the first one!
            // in the future the checksum should be assigned to pairs!
            if (!(*it)->checksum.empty())
                {
                    tupple.checksum = (*it)->checksum.front();
                    if (!params.isParamSet(JobParameterHandler::CHECKSUM_METHOD))
                        params.set(JobParameterHandler::CHECKSUM_METHOD, "relaxed");
                }

            // pair sources with destinations
            list< pair<string, string> > pairs = pairSourceAndDestination(
                    (*it)->source,
                    (*it)->dest,
                    tupple.selectionStrategy
            	);

            // if it is not multiple source/destination submission ..
            if (pairs.size() == 1)
                {
                    // add the source and destination SE for the transfer job
                    sourceSe = fileUrlToSeName(pairs.front().first);
                    destinationSe = fileUrlToSeName(pairs.front().second);
                }

            if (pairs.empty())
                {
                    throw Err_Custom("It has not been possible to pair the sources with destinations (protocols don't match)!");
                }

            // multiple pairs and reuse are not compatible!
            if (pairs.size() > 1 && params.get(JobParameterHandler::REUSE) == "Y")
                {
                    throw Err_Custom("Reuse and multiple replica selection are incompatible!");
                }

            // add each pair
            list< pair<string, string> >::iterator it_p;
            for (it_p = pairs.begin(); it_p != pairs.end(); it_p++)
                {
                    // check wether the destination file is supported
                    if (!checkProtocol(it_p->second))
                        {
                            throw Err_Custom("Destination protocol is not supported for file: " + it_p->second);
                        }
                    // check wether the source file is supported
                    if (!checkProtocol(it_p->first) && !checkIfLfn(it_p->first))
                        {
                            throw Err_Custom("Source protocol is not supported for file: " + it_p->first);
                        }
                    // set the values for source and destination
                    tupple.source = it_p->first;
                    tupple.destination = it_p->second;

                    if (!checkValidUrl(it_p->first) || !checkValidUrl(it_p->second))
                        {
                            std::string errMsg = "Not valid uri format, check submitted uri's";
                            throw Err_Custom(errMsg);
                        }


                    string sourceSe = fileUrlToSeName(it_p->first);
                    if(sourceSe.empty())
                        {
                            string errMsg = "Can't extract hostname from url " + it_p->first;
                            throw Err_Custom(errMsg);
                        }
                    checkSe(sourceSe, vo);
                    // check if all the sources use SRM protocol
                    srm_source &= sourceSe.find(srm_protocol) == 0;

                    string destinationSe = fileUrlToSeName(it_p->second);
                    if(destinationSe.empty())
                        {
                            std::string errMsg = "Can't extract hostname from url " + it_p->second;
                            throw Err_Custom(errMsg);
                        }
                    checkSe(destinationSe, vo);

                    tupple.source_se = sourceSe;
                    tupple.dest_se = destinationSe;

                    // check the timeout in case the source has been blacklisted
                    boost::optional<int> source_timeout = db->getTimeoutForSe(sourceSe);
                    // check the timeout in case the destination has been blacklisted
                    boost::optional<int> destin_timeout = db->getTimeoutForSe(destinationSe);

                    // set the wait_timeout for the transfer (in case the source / destination have been blacklisted with '--allow-submit')
                    if (source_timeout.is_initialized() && destin_timeout.is_initialized())
                        {
                            tupple.wait_timeout = *source_timeout < *destin_timeout ? *source_timeout : *destin_timeout;
                        }
                    else if (source_timeout.is_initialized())
                        {
                            tupple.wait_timeout = source_timeout;
                        }
                    else if (destin_timeout.is_initialized())
                        {
                            tupple.wait_timeout = destin_timeout;
                        }

                    tupple.activity = getActivity(tupple.metadata);

                    jobs.push_back(tupple);
                }
        }
}

void JobSubmitter::init(tns3__TransferParams *jobParams)
{

    id = UuidGenerator::generateUUID();
    FTS3_COMMON_LOGGER_NEWLOG (DEBUG) << "Generated uuid " << id << commit;

    if (jobParams)
        {
            params(jobParams->keys, jobParams->values);
            //FTS3_COMMON_LOGGER_NEWLOG (DEBUG) << "Parameter map has been created" << commit;
        }
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

    //send state message
    SingleTrStateInstance::instance().sendStateMessage(id, -1);

    /* Leave it as is just in case
    vector<job_element_tupple>::const_iterator it;
    for (it = jobs.begin(); it != jobs.end(); ++it) {
    SingleTrStateInstance::instance().sendStateMessage(vo, (*it).source_se, (*it).dest_se, id, -1, "SUBMITTED", "SUBMITTED", 0,
    params.get<int>(JobParameterHandler::RETRY), params.get<string>(JobParameterHandler::JOB_METADATA), (*it).metadata );
    }
    */


    FTS3_COMMON_LOGGER_NEWLOG (INFO) << "The jobid " << id << " has been submitted successfully" << commit;
    return id;
}

bool JobSubmitter::checkProtocol(string file)
{
    string tmp (file);
    transform(tmp.begin(), tmp.end(), tmp.begin(), ::tolower);

    return tmp.find("root://") == 0 || tmp.find("srm://") == 0 ||
           tmp.find("gsiftp://") == 0 || tmp.find("https://") == 0 ||
           tmp.find("lfc://") == 0;
}

bool JobSubmitter::checkIfLfn(string file)
{
    return file.find("/") == 0 && file.find(";") == string::npos && file.find(":") == string::npos;
}

string JobSubmitter::fileUrlToSeName(string url)
{

    smatch what;
    if (regex_match(url, what, fileUrlRegex, match_extra))
        {
            // indexes are shifted by 1 because at index 0 is the whole string
            return string(what[1]);

        }
    else
        return string();
}

void JobSubmitter::checkSe(string se, string vo)
{

    // check if the SE is blacklisted
    if (db->isSeBlacklisted(se, vo))
        {
            if (!db->allowSubmitForBlacklistedSe(se)) throw Err_Custom("The SE: " + se + " is blacklisted!");
        }
    // if we don't care about MyOSQ return
    if (!theServerConfig().get<bool>("MyOSG")) return;

    // checking of a state (active, disabled) in MyOSG  is commented out for now

//	// load from local file which is update by a cron job
//	OsgParser& osg = OsgParser::getInstance();
//	// check in the OSG if the SE is 'active'
//	optional<bool> state = osg.isActive(se);
//	if (state.is_initialized() && !(*state)) throw Err_Custom("The SE: " + se + " is not active in the OSG!");
//	// check in the OSG if the SE is 'disabled'
//	state = osg.isDisabled(se);
//	if (state.is_initialized() && *state) throw Err_Custom("The SE: " + se + " is disabled in the OSG!");

}

list< pair<string, string> > JobSubmitter::pairSourceAndDestination(
    vector<string> sources,
    vector<string> destinations,
    string selectionStrategy
)
{

    if (!selectionStrategy.empty() && selectionStrategy != "orderly" && selectionStrategy != "auto")
        throw Err_Custom("'" + selectionStrategy + "'");

    list< pair<string, string> > ret;

    // if it is single source - single destination submission just return the pair
    if (sources.size() == 1 && destinations.size() == 1)
        {
            ret.push_back(
                make_pair(sources.front(), destinations.front())
            );

            return ret;
        }

    vector<string>::iterator it_s, it_d;

    for (it_s = sources.begin(); it_s != sources.end(); it_s++)
        {
            // retrieve the protocol
            string::size_type pos = it_s->find("://");
            string protocol = it_s->substr(0, pos);
            // look for the corresponding destination
            for (it_d = destinations.begin(); it_d != destinations.end(); it_d++)
                {
                    // if the destination uses the same protocol ...
                    if (it_d->find(protocol) == 0 || it_d->find(srm_protocol) == 0 || protocol == srm_protocol)   // TODO verify!!!
                        {
                            ret.push_back(
                                make_pair(*it_s, *it_d)
                            );
                        }
                }
        }

    return ret;
}


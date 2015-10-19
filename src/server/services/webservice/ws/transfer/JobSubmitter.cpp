/*
 * Copyright (c) CERN 2013-2015
 *
 * Copyright (c) Members of the EMI Collaboration. 2010-2013
 *  See  http://www.eu-emi.eu/partners for details on the copyright
 *  holders.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "JobSubmitter.h"

#include "TransferCreator.h"
#include "PlainOldJob.h"

#include "db/generic/SingleDbInstance.h"

#include "../CGsiAdapter.h"
#include "../delegation/GSoapDelegationHandler.h"
#include "../config/Configuration.h"

#include "profiler/Macros.h"

#include <boost/lexical_cast.hpp>

#include <algorithm>
#include <numeric>

#include <boost/lambda/lambda.hpp>
#include <boost/optional.hpp>
#include <boost/assign.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/tuple/tuple.hpp>

#include "common/Exceptions.h"
#include "config/ServerConfig.h"
#include "common/Logger.h"
#include "common/Uri.h"
#include "common/UuidGenerator.h"
#include "../SingleTrStateInstance.h"

using namespace db;
using namespace fts3::config;
using namespace fts3::ws;
using namespace boost;
using namespace boost::assign;


const std::string JobSubmitter::srm_protocol = "srm";

static Uri checkValidUrl(const std::string &uri)
{
    Uri u0 = Uri::parse(uri);
    bool ok = u0.host.length() != 0 && u0.protocol.length() != 0 && u0.path.length() != 0 && u0.protocol.compare("file") != 0;
    if (!ok)
        {
            std::string errMsg = "Not valid uri format, check submitted uri's";
            throw UserError(errMsg);
        }
    return u0;
}

JobSubmitter::JobSubmitter(soap* ctx, tns3__TransferJob *job, bool delegation) :
    db (DBSingleton::instance().getDBObjectInstance()),
    copyPinLifeTime(-1),
    srm_source(true)
{
    PROFILE_SCOPE("JobSubmitter::JobSubmitter(soap*, tns3__TransferJob*, bool)");

    // do the common initialisation
    init(ctx, job);

    // check the delegation and MyProxy password settings
    if (delegation)
        {
            if (job->credential)
                {
                    throw UserError("The MyProxy password should not be provided if delegation is used");
                }
        }

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

    // do the common initialisation
    init(ctx, job);

    // checksum uses always delegation?
    if (job->credential)
        {
            throw UserError("The MyProxy password should not be provided if delegation is used");
        }


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
    for (auto it = job->transferJobElements.begin(); it < job->transferJobElements.end(); it++)
        {
            tns3__TransferJobElement3* elem = (*it);

            // prepare the job element and add it to the job
            SubmittedTransfer tupple;

            // common properties
            tupple.filesize = elem->filesize ? *elem->filesize : 0;
            tupple.metadata = elem->metadata ? *elem->metadata : std::string();
            tupple.activity = getActivity(elem->activity);

            tupple.selectionStrategy = elem->selectionStrategy ? *elem->selectionStrategy : std::string();
            if (!tupple.selectionStrategy.empty() && tupple.selectionStrategy != "orderly" && tupple.selectionStrategy != "auto")
                throw UserError("'" + tupple.selectionStrategy + "'");

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
                    throw UserError("Reuse and multiple replica selection are incompatible!");
                }

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

                    tupple.sourceSe = sourceSe;
                    tupple.destSe = destinationSe;

                    jobs.push_back(tupple);
                }
        }

    inspector.inspect();
    inspector.setWaitTimeout(jobs);
}

template <typename JOB>
void JobSubmitter::init(soap* ctx, JOB* job)
{

    // check wether the job is well specified
    if (job == 0 || job->transferJobElements.empty())
        {
            throw UserError("The job was not defined or job file is empty?");
        }


    GSoapDelegationHandler handler (ctx);
    delegationId = handler.makeDelegationId();

    CGsiAdapter cgsi (ctx);
    vo = cgsi.getClientVo();
    dn = cgsi.getClientDn();

    FTS3_COMMON_LOGGER_NEWLOG (INFO) << "DN: " << dn << " is submitting a transfer job" << commit;

    if (db->isDnBlacklisted(dn))
        {
            throw UserError("The DN: " + dn + " is blacklisted!");
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

inline std::string JobSubmitter::getActivity(const std::string * activity)
{
    // default value returned if the metadata are empty or an activity was not specified
    static const std::string defstr = "default";
    // if the activity was not specified return default
    if (!activity) return defstr;
    // if the activity was not found return default
    return *activity;
}

JobSubmitter::~JobSubmitter()
{

}

std::string JobSubmitter::submit()
{

    // for backwards compatibility check if copy-pin-lifetime and bring-online were set properly
    if (!params.isParamSet(JobParameterHandler::COPY_PIN_LIFETIME))
        {
            params.set(JobParameterHandler::COPY_PIN_LIFETIME, "-1");
        }
    else
        {
            // make sure that bring online has been used for SRM source
            // (bring online is not supported for multiple source/destination submission)
            if (params.get(JobParameterHandler::COPY_PIN_LIFETIME) != "-1" && !srm_source)
                throw UserError("The 'ping-lifetime' operation can be used only with source SEs that are using SRM protocol!");
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
                throw UserError("The 'bring-online' operation can be used only with source SEs that are using SRM protocol!");
        }

    if (!params.isParamSet(JobParameterHandler::RETRY))
        {
            params.set(JobParameterHandler::RETRY, "0");
        }

    if (!params.isParamSet(JobParameterHandler::RETRY_DELAY))
        {
            params.set(JobParameterHandler::RETRY_DELAY, "0");
        }

    if (params.get(JobParameterHandler::MULTIHOP) == "Y")
        {
            sourceSe = jobs.front().sourceSe;
            destinationSe = jobs.back().destSe;
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

    try
        {

            // submit the transfer job (add it to the DB)
            db->submitPhysical (
                id,
                jobs,
                dn,
                params.get(JobParameterHandler::CREDENTIALS),
                vo,
                std::string(),
                delegationId,
                sourceSe,
                destinationSe,
                params
            );
        }
    catch(BaseException& ex)
        {

            try
                {
                boost::this_thread::sleep(boost::posix_time::seconds(1));

                    // submit the transfer job (add it to the DB)
                    db->submitPhysical (
                        id,
                        jobs,
                        dn,
                        params.get(JobParameterHandler::CREDENTIALS),
                        vo,
                        std::string(),
                        delegationId,
                        sourceSe,
                        destinationSe,
                        params
                    );
                }
            catch(BaseException& ex)
                {

                    FTS3_COMMON_LOGGER_NEWLOG (ERR) << "An exception has been caught: " << ex.what() << commit;
                    throw UserError(std::string(__func__) + ": Caught exception " + ex.what());
                }
            catch(...)
                {
                    FTS3_COMMON_LOGGER_NEWLOG (ERR) << "An exception has been caught: db->submitPhysical"  << commit;
                    throw UserError(std::string(__func__) + ": Caught exception " );
                }
        }
    catch(...)
        {
            try
                {
                boost::this_thread::sleep(boost::posix_time::seconds(1));
                    // submit the transfer job (add it to the DB)
                    db->submitPhysical (
                        id,
                        jobs,
                        dn,
                        params.get(JobParameterHandler::CREDENTIALS),
                        vo,
                        std::string(),
                        delegationId,
                        sourceSe,
                        destinationSe,
                        params
                    );
                }
            catch(BaseException& ex)
                {

                    FTS3_COMMON_LOGGER_NEWLOG (ERR) << "An exception has been caught: " << ex.what() << commit;
                    throw UserError(std::string(__func__) + ": Caught exception " + ex.what());
                }
            catch(...)
                {
                    FTS3_COMMON_LOGGER_NEWLOG (ERR) << "An exception has been caught: db->submitPhysical"  << commit;
                    throw UserError(std::string(__func__) + ": Caught exception " );
                }
        }


    //send state message - disabled for now but pls do not remove it
    //SingleTrStateInstance::instance().sendStateMessage(id, -1);

    FTS3_COMMON_LOGGER_NEWLOG (INFO) << "The jobid " << id << " has been submitted successfully" << commit;
    return id;
}

void JobSubmitter::checkProtocol(std::string file, bool source)
{
    std::string tmp (file);
    transform(tmp.begin(), tmp.end(), tmp.begin(), ::tolower);
    trim(tmp);

    bool not_ok =
        // check protocol
        !(tmp.find("mock://") == 0 || tmp.find("root://") == 0 || tmp.find("srm://") == 0 || tmp.find("gsiftp://") == 0 ||
          tmp.find("https://") == 0 || tmp.find("lfc://") == 0 || tmp.find("davs://") == 0)
        &&
        // check if lfn if it is the source
        (!source || !(file.find("/") == 0 && file.find(";") == std::string::npos && file.find(":") == std::string::npos))
        ;

    if(not_ok)
        {
            std::string msg = (source ? "Source" : "Destination");
            msg += " protocol is not supported for file: "  + file;
            throw UserError(msg);
        }
}

std::string JobSubmitter::fileUrlToSeName(std::string url, bool source)
{
    Uri u0 = checkValidUrl(url);
    return u0.getSeName();
}


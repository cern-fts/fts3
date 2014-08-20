/*
 *	Copyright notice:
 *	Copyright © Members of the EMI Collaboration, 2010.
 *
 *	See www.eu-emi.eu for details on the copyright holders
 *
 *	Licensed under the Apache License, Version 2.0 (the "License");
 *	you may not use soap file except in compliance with the License.
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
 * RequestLister.cpp
 *
 *  Created on: Mar 9, 2012
 *      Author: Michał Simon
 */

#include "RequestLister.h"
#include "GSoapJobStatus.h"



#include "common/error.h"
#include "common/logger.h"
#include "common/JobStatusHandler.h"

using namespace db;
using namespace fts3::ws;
using namespace fts3::common;

RequestLister::RequestLister(::soap* soap, impltns__ArrayOf_USCOREsoapenc_USCOREstring *inGivenStates):
    soap(soap),
    cgsi(soap),
    db(*DBSingleton::instance().getDBObjectInstance())
{

    FTS3_COMMON_LOGGER_NEWLOG (INFO) << "DN: " << cgsi.getClientDn() << " is listing transfer job requests" << commit;
    checkGivenStates (inGivenStates);
}

RequestLister::RequestLister(::soap* soap, impltns__ArrayOf_USCOREsoapenc_USCOREstring *inGivenStates, string dn, string vo, string src, string dst):
    soap(soap),
    cgsi(soap),
    dn(dn),
    vo(vo),
    src(src),
    dst(dst),
    db(*DBSingleton::instance().getDBObjectInstance())
{

    FTS3_COMMON_LOGGER_NEWLOG (INFO) << "DN: " << cgsi.getClientDn() << " is listing transfer job requests" << commit;
    checkGivenStates (inGivenStates);
}

RequestLister::~RequestLister()
{

}

impltns__ArrayOf_USCOREtns3_USCOREJobStatus* RequestLister::list(AuthorizationManager::Level lvl)
{

    switch(lvl)
        {
        case AuthorizationManager::PRV:
            dn = cgsi.getClientDn();
            vo = cgsi.getClientVo();
            break;
        case AuthorizationManager::VO:
            vo = cgsi.getClientVo();
            break;
        }

    try
        {
            db.listRequests(jobs, inGivenStates, "", dn, vo, src, dst);
            db.listRequestsDm(jobs, inGivenStates, "", dn, vo, src, dst);
            FTS3_COMMON_LOGGER_NEWLOG (DEBUG) << "Job's statuses have been read from the database" << commit;
        }
    catch(Err& ex)
        {
            FTS3_COMMON_LOGGER_NEWLOG (ERR) << "An exception has been caught: " << ex.what() << commit;
            throw Err_Custom(std::string(__func__) + ": Caught exception " + ex.what());
        }
    catch(...)
        {
            FTS3_COMMON_LOGGER_NEWLOG (ERR) << "An exception has been caught: RequestLister::list"  << commit;
            throw Err_Custom(std::string(__func__) + ": Caught exception " );
        }

    // create the object
    impltns__ArrayOf_USCOREtns3_USCOREJobStatus* result;
    result = soap_new_impltns__ArrayOf_USCOREtns3_USCOREJobStatus(soap, -1);

    // fill it with job statuses
    vector<JobStatus*>::iterator it;
    for (it = jobs.begin(); it < jobs.end(); ++it)
        {
            GSoapJobStatus job_ptr (soap, **it);
            result->item.push_back(job_ptr);
            delete *it;
        }
    FTS3_COMMON_LOGGER_NEWLOG (DEBUG) << "The response has been created" << commit;

    return result;
}

void RequestLister::checkGivenStates(impltns__ArrayOf_USCOREsoapenc_USCOREstring* inGivenStates)
{

    if (!inGivenStates || inGivenStates->item.empty())
        {
            throw Err_Custom("No states were defined!");
        }

    JobStatusHandler& handler = JobStatusHandler::getInstance();
    vector<string>::iterator it;
    for (it = inGivenStates->item.begin(); it < inGivenStates->item.end(); ++it)
        {
            if (*it == "Pending") continue; // TODO for now we are ignoring the legacy state 'Pending'
            if(!handler.isStatusValid(*it))
                {
                    throw Err_Custom("Unknown job status: " + *it);
                }
        }

    this->inGivenStates = inGivenStates->item;
}

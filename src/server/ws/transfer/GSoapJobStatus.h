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
 * GSoapJobStatus.h
 *
 *  Created on: Dec 17, 2012
 *      Author: simonm
 */

#ifndef GSOAPJOBSTATUS_H_
#define GSOAPJOBSTATUS_H_

#include "ws-ifce/gsoap/gsoap_stubs.h"

namespace fts3
{
namespace ws
{

class GSoapJobStatus
{

public:
    template <typename STATUS>
    GSoapJobStatus(soap* ctx, STATUS& status)
    {

        me = soap_new_tns3__JobStatus(ctx, -1);

        me->clientDN = soap_new_std__string(ctx, -1);
        *me->clientDN = status.clientDN;

        me->jobID = soap_new_std__string(ctx, -1);
        *me->jobID = status.jobID;

        me->jobStatus = soap_new_std__string(ctx, -1);
        *me->jobStatus = status.jobStatus;

        me->reason = soap_new_std__string(ctx, -1);
        *me->reason = status.reason;

        me->voName = soap_new_std__string(ctx, -1);
        *me->voName = status.voName;

        // change sec precision to msec precision so it is compatible with old glite cli
        me->submitTime = status.submitTime * 1000;
        me->numFiles = status.numFiles;
        me->priority = status.priority;
    }


    virtual ~GSoapJobStatus() {};

    operator tns3__JobStatus*() const
    {
        return me;
    }

private:
    tns3__JobStatus* me;
};

} /* namespace ws */
} /* namespace fts3 */
#endif /* GSOAPJOBSTATUS_H_ */

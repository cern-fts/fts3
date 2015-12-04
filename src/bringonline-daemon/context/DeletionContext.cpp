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

#include "DeletionContext.h"
#include "cred/CredUtility.h"


void DeletionContext::add(const DeleteOperation &nsOp)
{
    add(nsOp.surl, nsOp.jobId, nsOp.fileId);
}


void DeletionContext::updateState(const std::string &jobId, int fileId, const std::string &state,
    const std::string &reason, bool retry) const
{
    stateUpdater(jobId, fileId, state, reason, retry);
}


void DeletionContext::updateState(const std::string &state, const std::string &reason,
    bool retry) const
{
    stateUpdater(jobs, state, reason, retry);
}

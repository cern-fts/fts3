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

#include "CDMIQosTransitionContext.h"

#include <sstream>
#include <unordered_set>
#include "cred/CredUtility.h"


void CDMIQosTransitionContext::add(const QosTransitionOperation &qosTransitionOp)
{
    if (!qosTransitionOp.surl.empty() && !qosTransitionOp.jobId.empty() && !qosTransitionOp.token.empty() && !qosTransitionOp.target_qos.empty()) {
    	//std::cerr << "About to add file: " << qosTransitionOp.surl <<  std::endl;
    	//std::cerr << "size before: " << filesToTransition[qosTransitionOp.jobId].size() <<  std::endl;
    	filesToTransition[qosTransitionOp.jobId].push_back(qosTransitionOp);
    	//std::cerr << "size after: " << filesToTransition[qosTransitionOp.jobId].size() <<  std::endl;
    	//for (int i=0; i < filesToTransition[qosTransitionOp.jobId].size(); i++) {
    	//	std::cerr << "This is the stuff " << filesToTransition[qosTransitionOp.jobId].at(i).surl << std::endl;
    	//}
    }
}

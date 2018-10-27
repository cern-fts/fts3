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

#include "common/Exceptions.h"
#include "common/Logger.h"

#include "QoSTransitionTask.h"
#include "PollTask.h"
#include "WaitingRoom.h"


boost::shared_mutex QoSTransitionTask::mx;

std::set<std::tuple<std::string, std::string, std::string, std::string, uint64_t>> QoSTransitionTask::active_surls;

void QoSTransitionTask::run(const boost::any &)
{
	//std::cerr << "This is the set size: " << active_surls.size() << std::endl;
	std::vector<GError*> errors(active_surls.size(), NULL);
	for (auto it = active_surls.begin(); it != active_surls.end(); ++it) {
	      //std::cerr << "Printing surl: " << std::get<0>(*it) << std::endl;
	      //std::cerr << "Printing token: " << std::get<1>(*it) << std::endl;
	      //std::cerr << "Printing target_qos: " << std::get<2>(*it) << std::endl;
	      //std::cerr << "Printing job_id: " << std::get<3>(*it) << std::endl;
	      //std::cerr << "Printing file_id: " << std::get<4>(*it) << std::endl;
		 //std::cerr << "The host is:" << Uri::parse(std::get<0>(*it).c_str()).host << std::endl;

	      GError *err = NULL;

	      // Add token to context
	      gfal2_cred_t *cred = gfal2_cred_new(GFAL_CRED_BEARER, std::get<1>(*it).c_str());
	      gfal2_cred_set(gfal2_ctx, Uri::parse(std::get<0>(*it)).host.c_str(), cred, &err);

	      // Perform transition
	      std::cerr << "Perform QoS transition of " << std::get<0>(*it) << " to QoS: " << std::get<2>(*it) << std::endl;
	      // TODO: add error checking
		  int result = gfal2_change_object_qos(gfal2_ctx, std::get<0>(*it).c_str(), std::get<2>(*it).c_str(), &err);

	      //Delete token from context
		  gfal2_cred_free(cred);
		  gfal2_cred_clean(gfal2_ctx, &err);

		  if (result != -1) {
			  std::cerr << "QoS Transition of " << std::get<0>(*it) << " to " << std::get<2>(*it) << " successful" << std::endl;
			  db::DBSingleton::instance().getDBObjectInstance()->updateFileStateToFinished(std::get<3>(*it), std::get<4>(*it));
		  } else {
			  std::cerr << "QoS Transition of " << std::get<0>(*it) << " to " << std::get<2>(*it) << " failed" << std::endl;
		  }
	}
}

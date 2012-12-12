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
 *	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *	See the License for the specific language governing permissions and
 *	limitations under the License.
 * GSoapContextAdapter.h
 *
 * FileTransferScheduler.cpp
 *
 *  Created on: May 7, 2012
 *      Author: Michał Simon
 */


#include <boost/lexical_cast.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/scoped_ptr.hpp>

#include <sstream>

#include "FileTransferScheduler.h"

#include "ws/config/Configuration.h"

#include "common/logger.h"
#include "ws-ifce/JobStatusHandler.h"


using namespace db;
using namespace fts3::ws;

string FileTransferScheduler::fileUrlToSeName(string url) {

	static const regex re(".+://([a-zA-Z0-9\\.-]+)(:\\d+)?/.+");
	smatch what;

	if (regex_match(url, what, re, match_extra)) {
		// indexes are shifted by 1 because at index 0 is the whole string
		return string(what[1]);
	} else
		return string();
}

FileTransferScheduler::FileTransferScheduler(TransferFiles* file) :
		file (file), db (DBSingleton::instance().getDBObjectInstance())
		{

	srcSeName = fileUrlToSeName(file->SOURCE_SURL);
	destSeName = fileUrlToSeName(file->DEST_SURL);
}

FileTransferScheduler::~FileTransferScheduler() {

}

bool FileTransferScheduler::schedule(bool optimize, bool manual) {

	if(optimize && !manual) {
		bool allowed = db->isTrAllowed(srcSeName, destSeName);
		// update file state to READY
		if(allowed == true) {
			unsigned updated = db->updateFileStatus(file, JobStatusHandler::FTS3_STATUS_READY);
			if(updated == 0) return false;
			return true;
		}
		return false;
	}

	// source, destination & VO
	vector< tuple<string, string, string> > cfgs = db->getJobShareConfig(file->JOB_ID);
	vector< tuple<string, string, string> >::iterator it;

	// get the number of configurations assigned to the transfer job
	optional<int> count = db->getJobConfigCount(file->JOB_ID);
	// check if count has been initialized, in principal it must been initialized, but anyway it is good to check
	if (count) {
		if ( (*count != cfgs.size()) && manual) {
			// the configuration has been removed,
			// return false in next round configuration will be reassigned
			// or auto-tuner will take care of it
			return false;
		}
	}

	for (it = cfgs.begin(); it != cfgs.end(); it++) {

		string source = get<SOURCE>(*it);
		string destination = get<DESTINATION>(*it);
		string vo = get<VO>(*it);

		scoped_ptr<ShareConfig> cfg (
				db->getShareConfig(source, destination, vo)
			);

		if (!cfg.get()) continue; // if the configuration has been deleted in the meanwhile continue

		// check if the configuration allows this type of transfer-job
		if (!cfg->active_transfers) {
			// failed to allocate active transfers credits to transfer-job
			string msg = getNoCreditsErrMsg(cfg.get());
			// set file status to failed
			db->updateFileTransferStatus(
					file->JOB_ID,
					lexical_cast<string>(file->FILE_ID),
					JobStatusHandler::FTS3_STATUS_FAILED,
					msg,
					0,
					0,
					0
				);
			// set job states if necessary
			db->updateJobTransferStatus(
					lexical_cast<string>(file->FILE_ID),
					file->JOB_ID,
					JobStatusHandler::FTS3_STATUS_FAILED
				);
			// log it
			FTS3_COMMON_LOGGER_NEWLOG (INFO) << msg << commit;
			// the file has been resolved as FAILED, it won't be scheduled
			return false;
		}

		int active_transfers = 0;

		if (source == Configuration::wildcard) {
			active_transfers = db->countActiveOutboundTransfersUsingDefaultCfg(srcSeName, vo);
			if (cfg->active_transfers - active_transfers > 0) continue;
			return false;
		}

		if (destination == Configuration::wildcard) {
			active_transfers = db->countActiveInboundTransfersUsingDefaultCfg(destSeName, vo);
			if (cfg->active_transfers - active_transfers > 0) continue;
			return false;
		}

		active_transfers = db->countActiveTransfers(source, destination, vo);
		if (cfg->active_transfers - active_transfers > 0) continue;
		return false;
	}

	// update file state to READY
	unsigned updated = db->updateFileStatus(file, JobStatusHandler::FTS3_STATUS_READY);
	if(updated == 0)
		return false;

	return true;
}

string FileTransferScheduler::getNoCreditsErrMsg(ShareConfig* cfg) {
	stringstream ss;

	ss << "Failed to allocate active transfer credits to transfer job due to ";

	if (cfg->source == Configuration::wildcard || cfg->destination == Configuration::wildcard) {

		ss << "the default standalone SE configuration.";

	} else if (cfg->source != Configuration::any && cfg->destination != Configuration::any) {

		ss << "a pair configuration (" << cfg->source << " -> " << cfg->destination << ").";

	} else if (cfg->source != Configuration::any) {

		ss << "a standalone out-bound configuration (" << cfg->source << ").";

	} else if (cfg->destination != Configuration::any) {

		ss << "a standalone in-bound configuration (" << cfg->destination << ").";

	} else {

		ss << "configuration constraints.";
	}

	ss << "Only the following VOs are allowed: ";

	vector<ShareConfig*> cfgs = db->getShareConfig(cfg->source, cfg->destination);
	vector<ShareConfig*>::iterator it;

	for (it = cfgs.begin(); it != cfgs.end(); it++) {
		scoped_ptr<ShareConfig> ptr (*it);
		if (ptr->active_transfers) {
			if (it != cfgs.begin()) ss << ", ";
			ss << ptr->vo;
		}
	}

	return ss.str();
}


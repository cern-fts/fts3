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

#include <sstream>

#include "FileTransferScheduler.h"

#include "ws/config/Configuration.h"

#include "common/logger.h"
#include "common/JobStatusHandler.h"


using namespace db;
using namespace fts3::ws;


FileTransferScheduler::FileTransferScheduler(TransferFiles* file, vector< shared_ptr<ShareConfig> >& cfgs) :
		file (file),
		cfgs (cfgs),
		db (DBSingleton::instance().getDBObjectInstance()) {

	srcSeName = file->SOURCE_SE;
	destSeName = file->DEST_SE;
}

FileTransferScheduler::~FileTransferScheduler() {

}

bool FileTransferScheduler::schedule(bool optimize) {

	if(optimize && cfgs.empty()) {
		bool allowed = db->isTrAllowed(srcSeName, destSeName);
		// update file state to READY
		if(allowed == true) {
			unsigned updated = db->updateFileStatus(file, JobStatusHandler::FTS3_STATUS_READY);
			if(updated == 0) return false;
			// set all other files that were generated due to a multi-source/destination submission to NOT_USED
			db->setFilesToNotUsed(file->JOB_ID, file->FILE_INDEX);
			return true;
		}
		return false;
	}

	vector< shared_ptr<ShareConfig> >::iterator it;

	for (it = cfgs.begin(); it != cfgs.end(); ++it) {

		string source = (*it)->source;
		string destination = (*it)->destination;
		string vo = (*it)->vo;

		shared_ptr<ShareConfig>& cfg = *it;

		if (!cfg.get()) continue; // if the configuration has been deleted in the meanwhile continue

		// check if the configuration allows this type of transfer-job
		if (!cfg->active_transfers) {
			// failed to allocate active transfers credits to transfer-job
			string msg = getNoCreditsErrMsg(cfg.get());
			// set file status to failed
			db->updateFileTransferStatus(
					file->JOB_ID,
					file->FILE_ID,
					JobStatusHandler::FTS3_STATUS_FAILED,
					msg,
					0,
					0,
					0
				);
			// set job states if necessary
			db->updateJobTransferStatus(
					file->FILE_ID,
					file->JOB_ID,
					JobStatusHandler::FTS3_STATUS_FAILED
				);
			// log it
			FTS3_COMMON_LOGGER_NEWLOG (ERR) << msg << commit;
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
	// set all other files that were generated due to a multi-source/destination submission to NOT_USED
	db->setFilesToNotUsed(file->JOB_ID, file->FILE_INDEX);

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

	for (it = cfgs.begin(); it != cfgs.end(); ++it) {
		shared_ptr<ShareConfig> ptr (*it);
		if (ptr->active_transfers) {
			if (it != cfgs.begin()) ss << ", ";
			ss << ptr->vo;
		}
	}

	return ss.str();
}


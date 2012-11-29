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

const regex FileTransferScheduler::fileUrlRegex(".+://([a-zA-Z0-9\\.-]+)(:\\d+)?/.+");

string FileTransferScheduler::fileUrlToSeName(string url) {

	smatch what;
	if (regex_match(url, what, fileUrlRegex, match_extra)) {

		// indexes are shifted by 1 because at index 0 is the whole string
		return string(what[1]);

	} else
		return string();
}

FileTransferScheduler::FileTransferScheduler(TransferFiles* file) :
		db (DBSingleton::instance().getDBObjectInstance()),
		file (file) {

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

	for (it = cfgs.begin(); it != cfgs.end(); it++) {

		string source = get<SOURCE>(*it);
		string destination = get<DESTINATION>(*it);
		string vo = get<VO>(*it);

		scoped_ptr<ShareConfig> cfg (
				db->getShareConfig(source, destination, vo)
			);

		if (!cfg.get()) continue; // if the configuration has been deleted in the meanwhile continue

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


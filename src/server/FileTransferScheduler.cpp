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
#include "boost/tuple/tuple.hpp"

#include <sstream>

#include "FileTransferScheduler.h"

#include "common/logger.h"
#include "common/JobStatusHandler.h"


using namespace db;

//const string FileTransferScheduler::seNameRegex = ".+://([a-zA-Z0-9\\.-]+)(:\\d+)?/.+";

FileTransferScheduler::FileTransferScheduler(TransferFiles* file, vector<TransferFiles*> otherFiles) :
		db (DBSingleton::instance().getDBObjectInstance()) {

	if(file){
		this->file = file;
	// prepare input for SE part
	// TODO check if initialized
	srcSeName = CfgBlocks::fileUrlToSeName(file->SOURCE_SURL).get();

	// TODO check if initialized
	destSeName = CfgBlocks::fileUrlToSeName(file->DEST_SURL).get();

	voName = file->VO_NAME;

	// prepare input for group part
	srcGroupName = db->get_group_name(srcSeName);
	destGroupName = db->get_group_name(destSeName);

	initVosInQueue(otherFiles);
       }	
}

FileTransferScheduler::~FileTransferScheduler() {

}

optional< tuple<int, int, string> > FileTransferScheduler::getValue(string type, string name, string shareId) {

	vector<SeAndConfig*> seAndConfig;

	// query the DB
	db->getAllShareAndConfigWithCritiria(
			seAndConfig,
			name,
			shareId,
			type,
			""
		);

	// the value to be returned
	optional< tuple<int, int, string> > val;

	// there can be only one entry in the vector (name + type + sharedId = primary key)
	if (!seAndConfig.empty()) {

		// parse the string
		val = CfgBlocks::getShareValue(
				(*seAndConfig.begin())->SHARE_VALUE
			);

		// free the memory
		delete *seAndConfig.begin();

	}

	return val;
}

int FileTransferScheduler::getCreditsInUse(IO io, Share share, const string type) {

	int creditsInUse = 0;

	string srcSeName, destSeName, srcGroupName, destGroupName, voName;

	switch (share) {
	// if we are interested in voshare set vo's name
	case VO:
		voName = this->voName;
		break;
	// if we are interested in pairshare set both src and dest
	case PAIR:
		srcSeName = this->srcSeName;
		srcGroupName = this->srcGroupName;
		destSeName = this->destSeName;
		destGroupName = this->destGroupName;
		break;
	// otherwise (e.g. public share) do nothing
	default: break;
	}

	switch (io) {
	// if we are interested in outbound set the src
	case CfgBlocks::OUTBOUND:
		srcSeName = this->srcSeName;
		srcGroupName = this->srcGroupName;
		break;
	// if we are interested in inbound set the dest
	case CfgBlocks::INBOUND:
		destSeName = this->destSeName;
		destGroupName = this->destGroupName;
		break;
	}

	// query for the credits in DB
	if (type == CfgBlocks::SE_TYPE) {
		// we are looking for SE credits
		db->getSeCreditsInUse (
				creditsInUse,
				srcSeName,
				destSeName,
				voName
			);

	} else {
		// we are looking for SE group credits
		db->getGroupCreditsInUse (
				creditsInUse,
				srcGroupName,
				destGroupName,
				voName
			);
	}

	return creditsInUse;
}

// TODO if a configuration has not been found for specific transfer it not necessary means
// that the transfer should go to ready state (?) because if there are other configurations
// it means the se not configured to accept this kind of transfers
int FileTransferScheduler::getFreeCredits(IO io, Share share, const string type, string name, string param) {

	stringstream log;

	// if the SE does not belong to a SE group
	if (name.empty() && type == CfgBlocks::GROUP_TYPE) {

		log << (io == INBOUND ? "Destination SE " : "Source SE ");
		log << "is not a member of a group therefore there are no group-related constraints";
		FTS3_COMMON_LOGGER_NEWLOG (INFO) << log.str() << commit;

		return FREE_CREDITS;
	}

	log << (type == CfgBlocks::SE_TYPE ? "SE: " : "SE group: ");
	log << name << " ";

	optional< tuple<int, int, string> > val;

	switch (share) {
	// we are looking for VO share
	case VO:
		val = getValue(type, name, CfgBlocks::voShare(param));
		break;
	// we are looking for PAIR share
	case PAIR:
		val = getValue(type, name, CfgBlocks::pairShare(param));
		// if there is no cfg there are no constraints
		if (!val.is_initialized()) {

			log << " has no pair-share configuration for ";
			log << param << " therefore there are no pair-related constraints";
			FTS3_COMMON_LOGGER_NEWLOG (INFO) << log.str() << commit;

			return FREE_CREDITS;
		}
		break;
	// otherwise do nothing
	default: break;
	}

	// if a cfg has been found, look for the number of credits in use, and return the difference
	if (val.is_initialized()) {

		int credits = getCredits(val.get(), io);
		int creditsInUse = getCreditsInUse(io, share, type);

		log << " has a ";
		log << (share == VO ? "vo-share " : "pair-share ");
		log << "configuration: " << credits;
		log << (io == INBOUND ? "inbound cerdits, " : "outbound credits, ");
		log << "currently " << creditsInUse << " in use";
		FTS3_COMMON_LOGGER_NEWLOG (INFO) << log.str() << commit;

		return  credits - creditsInUse;
	}

	log << " has no vo-share configuration";

	// look for the number of credits in use for public share (it will be the same for default)
	int creditsInUse = getCreditsInUse(io, PUBLIC, type);

	// look for public share
	val = getValue(type, name, CfgBlocks::publicShare());

	// if a cfg has been found, and return the difference
	if (val.is_initialized()) {

		int credits  = getCredits(val.get(), io);

		log << ", using public-share configuration: ";
		log << credits;
		log << (io == INBOUND ? " inbound cerdits, " : " outbound credits, ");
		log << "currently " << creditsInUse << " in use";
		FTS3_COMMON_LOGGER_NEWLOG (INFO) << log.str() << commit;

		return credits - creditsInUse;
	}

	log << " and no public-share configuration";

	if (type == CfgBlocks::GROUP_TYPE) {

		log << ", therefore there are no group-related constraints";
		FTS3_COMMON_LOGGER_NEWLOG (INFO) << log.str() << commit;

		return FREE_CREDITS;
	}

	log << ", using default configuration: ";
	log << DEFAULT_VALUE;
	log << (io == INBOUND ? " inbound cerdits, " : " outbound credits, ");
	log << "currently " << creditsInUse << " in use";
	FTS3_COMMON_LOGGER_NEWLOG (INFO) << log.str() << commit;

	// otherwise return the difference for the default value
	return DEFAULT_VALUE - creditsInUse;
}

bool FileTransferScheduler::schedule(bool optimize, bool manualConfig) {

	if(optimize == true && manualConfig==false) {
		bool allowed = db->isTrAllowed(srcSeName, destSeName);
		// update file state to READY
		if(allowed == true) {
			unsigned updated = db->updateFileStatus(file, JobStatusHandler::FTS3_STATUS_READY);
			if(updated == 0) {
				return false;
			} else {
				return true;
			}
		}
		return false;
	}

	if (getFreeCredits(OUTBOUND, VO, CfgBlocks::SE_TYPE, srcSeName, voName) <= 0) {
		return false;
	}

	if (getFreeCredits(INBOUND, VO, CfgBlocks::SE_TYPE, destSeName, voName) <= 0) {
		return false;
	}

	if (getFreeCredits(OUTBOUND, PAIR, CfgBlocks::SE_TYPE, srcSeName, destSeName) <= 0) {
		return false;
	}

	if (getFreeCredits(INBOUND, PAIR, CfgBlocks::SE_TYPE, destSeName, srcSeName) <= 0) {
		return false;
	}

	if (getFreeCredits(OUTBOUND, VO, CfgBlocks::GROUP_TYPE, srcGroupName, voName) <= 0) {
		return false;
	}

	if (getFreeCredits(INBOUND, VO, CfgBlocks::GROUP_TYPE, destGroupName, voName) <= 0) {
		return false;
	}

	if (getFreeCredits(OUTBOUND, PAIR, CfgBlocks::GROUP_TYPE, srcGroupName, destGroupName) <= 0) {
		return false;
	}

	if (getFreeCredits(INBOUND, PAIR, CfgBlocks::GROUP_TYPE, destGroupName, srcGroupName) <= 0) {
		return false;
	}

	// update file state to READY
	unsigned updated = db->updateFileStatus(file, JobStatusHandler::FTS3_STATUS_READY);
	if(updated == 0)
		return false;

	return true;
}

// SE only TODO
void FileTransferScheduler::initVosInQueue(vector<TransferFiles*> files) {

	vector<TransferFiles*>::iterator it;
	for (it = files.begin(); it < files.begin(); it++) {

		TransferFiles* tmp = *it;
		string srcSeName = CfgBlocks::fileUrlToSeName(tmp->SOURCE_SURL).get();
		string destSeName = CfgBlocks::fileUrlToSeName(tmp->DEST_SURL).get();

		if (srcSeName == this->srcSeName) {
			vosInQueueOut.insert(tmp->VO_NAME);
		}

		if (destSeName == this->destSeName) {
			vosInQueueIn.insert(tmp->VO_NAME);
		}
	}
}

// SE only TODO
int FileTransferScheduler::resolveSharedCredits(const string type, string name, Share, IO io) {

	// TODO assumption PAIR configuration don't support shared policy

	vector<SeAndConfig*> seAndConfig;
	// determine if there is public shared configuration for the SE
	db->getAllShareAndConfigWithCritiria (
			seAndConfig,
			name,
			CfgBlocks::publicShare(),
			type,
			CfgBlocks::shareValue(
					CfgBlocks::SQL_WILDCARD,
					CfgBlocks::SQL_WILDCARD,
					CfgBlocks::SHARED_POLICY
				)
		);

	bool hasPublicShared = !seAndConfig.empty();

	// all VOs that have a submitted file in the queue
	// if there is other policy this should be the vos that are using the credits!!!
	set<string> &vosInQueue = io == INBOUND ? vosInQueueIn : vosInQueueOut;

	// all VOs that have a pending file and use public-share
	set<string> vosUsingPublic;
	// we care only if the public-share has shared policy
	if (hasPublicShared) {
		// determined which VOs are using the public credits
		set<string>::iterator v_it;
		for (v_it = vosInQueue.begin(); v_it != vosInQueue.end(); v_it++) {
			db->getAllShareAndConfigWithCritiria (
					seAndConfig,
					name,
					CfgBlocks::voShare(*v_it),
					type,
					string()
				);

			if (seAndConfig.empty()) {
				// there is no vo-share configuration so it has to use public-share
				vosUsingPublic.insert(*v_it);
			}
		}
	}

	// select all values with for the given name and type and with shared policy
	db->getAllShareAndConfigWithCritiria (
			seAndConfig,
			name,
			string(),
			type,
			CfgBlocks::shareValue(
					CfgBlocks::SQL_WILDCARD,
					CfgBlocks::SQL_WILDCARD,
					CfgBlocks::SHARED_POLICY
				)
		);

	// sum of all the shared inbound/outbound credits
	int sumAll = 0;
	// sum of the inbound/outbound credits for submitted files in the queue
	int sumInQueue = 0;
	// inbound/outbound of the se/group (name)
	int credits = 0;

	vector<SeAndConfig*>::iterator it;
	for (it = seAndConfig.begin(); it < seAndConfig.end(); it++) {

		SeAndConfig* cfg = *it;

		// share id (vo or public)
		string share_id = cfg->SHARE_ID;
		// parse the share_id string
		optional< tuple<string, string> > id = CfgBlocks::getShareId(share_id);

		// vo or public
		string type = get<CfgBlocks::TYPE>(id.get());
		// vo name or empty if its public share
		string vo = get<CfgBlocks::ID>(id.get());

		// share value
		string share_value = cfg->SHARE_VALUE;
		// parse the share_value string
		optional< tuple<int, int, string> > val = CfgBlocks::getShareValue(share_value);

		int value;
		switch (io) {
		case CfgBlocks::INBOUND:
			value = get<CfgBlocks::INBOUND>(val.get());
			break;
		case CfgBlocks::OUTBOUND:
			value = get<CfgBlocks::OUTBOUND>(val.get());
			break;
		}

		sumAll += value;

		if (cfg->SE_NAME == name) {
			credits = value;
		}

		if (vo.empty()) {
			// it is public-share
			if(!vosUsingPublic.empty()) {
				// some VOs are using it
				sumInQueue += value;
			}
		} else if (vosInQueue.find(vo) != vosInQueue.end()) {
			// it is vo-share and the VO is not amongst VOs that have pending files
			sumInQueue += value;
		}
	}

	return credits / sumInQueue * (sumAll/*TODO - allSharedCreditsInUse*/);
}


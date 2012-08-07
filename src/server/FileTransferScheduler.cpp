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

#include "FileTransferScheduler.h"

#include "common/logger.h"
#include "common/JobStatusHandler.h"


using namespace db;

const string FileTransferScheduler::SE_TYPE = "se";
const string FileTransferScheduler::GROUP_TYPE = "group";

const string FileTransferScheduler::SHARED_POLICY = "shared";
const string FileTransferScheduler::EXCLUSIVE_POLICY = "exclusive";

const string FileTransferScheduler::getSharedValue = "\"in\":%,\"out\":%,\"policy\":\"shared\"";
const string FileTransferScheduler::shareIdRegex = "\"share_type\":\"(public|vo)\",\"share_id\":(null|\"(.+)\")";
const string FileTransferScheduler::shareValueRegex = "\"in\":(\\d+),\"out\":(\\d+),\"policy\":\"(\\w+)\"";
const string FileTransferScheduler::seNameRegex = ".+://([a-zA-Z0-9\\.-]+):\\d+/.+";

FileTransferScheduler::FileTransferScheduler(TransferFiles* file, vector<TransferFiles*> otherFiles) :
		db (DBSingleton::instance().getDBObjectInstance()) {

	this->file = file;

	// prepare input for SE part
	re.set_expression(seNameRegex);
	regex_match(file->SOURCE_SURL, what, re, match_extra);
	srcSeName = what[SE_NAME_REGEX_INDEX];

	regex_match(file->DEST_SURL, what, re, match_extra);
	destSeName = what[SE_NAME_REGEX_INDEX];

	voName = file->VO_NAME;

	// prepare input for group part
	srcGroupName = db->get_group_name(srcSeName);
	destGroupName = db->get_group_name(destSeName);

	initVosInQueue(otherFiles);
}

FileTransferScheduler::~FileTransferScheduler() {

}

FileTransferScheduler::Config FileTransferScheduler::getValue(string type, string name, string shareId) {

	FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Get configuration for: "
			<< type << ", "
			<< name << ", "
			<< shareId
			<< commit;

	Config ret;
	vector<SeAndConfig*> seAndConfig;

	// query the DB
	db->getAllShareAndConfigWithCritiria(
			seAndConfig,
			name,
			shareId,
			type,
			""
		);

	// there can be only one entry in the vector (name + type + sharedId = primary key)
	if (!seAndConfig.empty()) {

		// parse the string
		re.set_expression(shareValueRegex);
		regex_match((*seAndConfig.begin())->SHARE_VALUE, what, re, match_extra);

		// set the values
		ret.inbound = lexical_cast<int>(what[INBOUND_REGEX_INDEX]);
		ret.outbound = lexical_cast<int>(what[OUTBOUND_REGEX_INDEX]);
		ret.set = true;

		FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Inbound: " << ret.inbound << ", outbound: " << ret.outbound << commit;

		// free the memory
		delete *seAndConfig.begin();

	} else {
		FTS3_COMMON_LOGGER_NEWLOG(INFO) << "No configuration found." << commit;
	}

	return ret;
}

int FileTransferScheduler::getCreditsInUse(IO io, Share share, const string type) {

	FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Looking for credits in use for: "
			<< (io == INBOUND ? "inbound" : "outbound") << ", "
			<< (share == PAIR ? "pair" : share == VO ? "vo" : "public") << "share, "
			<< (type == SE_TYPE ? "SE" : "SE group")
			<< commit;

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
	case OUTBOUND:
		srcSeName = this->srcSeName;
		srcGroupName = this->srcGroupName;
		break;
	// if we are interested in inbound set the dest
	case INBOUND:
		destSeName = this->destSeName;
		destGroupName = this->destGroupName;
		break;
	}

	// query for the credits in DB
	if (type == SE_TYPE) {
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

	FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Credits in use: " << creditsInUse << commit;

	return creditsInUse;
}

int FileTransferScheduler::getFreeCredits(IO io, Share share, const string type, string name, string param) {

	// if the SE does not belong to a SE group
	if (name.empty()) return FREE_CREDITS;

	FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Looking for: "
			<< (io == OUTBOUND ? "outbound" : "inbound") << " credits, "
			<< (share == VO ? "vo" : "pair") << "share: " << param << ", "
			<< (type == SE_TYPE ? "SE" : "SE group") << ": " << name
			<< commit;


	Config cfg;

	switch (share) {
	// we are looking for VO share
	case VO:
		cfg = getValue(type, name, voShare(param));
		break;
	// we are looking for PAIR share
	case PAIR:
		cfg = getValue(type, name, pairShare(param));
		// if there is no cfg there are no constraints
		if (!cfg) {
			FTS3_COMMON_LOGGER_NEWLOG(INFO) << "There are no constraints in the DB in respect to the "
					<< (type == SE_TYPE ? "SE" : "SE group") << "-pair configuration."
					<< commit;
			return FREE_CREDITS;
		}
		break;
	// otherwise do nothing
	default: break;
	}

	// if a cfg has been found, look for the number of credits in use, and return the difference
	if (cfg) return cfg[io] - getCreditsInUse(io, share, type);

	// look for the number of credits in use for public share (it will be the same for default)
	int creditsInUse = getCreditsInUse(io, PUBLIC, type);

	FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Looking for: "
			<< (io == OUTBOUND ? "outbound" : "inbound") << " credits, "
			<< "publicshare, "
			<< (type == SE_TYPE ? "SE" : "SE group") << ": " << name
			<< commit;
	// look for public share
	cfg = getValue(type, name, publicShare());

	// if a cfg has been found, and return the difference
	if (cfg) return cfg[io] - creditsInUse;

	if (type == GROUP_TYPE) {
		FTS3_COMMON_LOGGER_NEWLOG(INFO) << "There are no SE group settings in the DB!" << commit;
		return FREE_CREDITS;
	}

	int defval = DEFAULT_VALUE;
	FTS3_COMMON_LOGGER_NEWLOG(INFO) << "The default value (" << defval << ") is used." << commit;
	// otherwise return the difference for the default value
	return DEFAULT_VALUE - creditsInUse;
}

bool FileTransferScheduler::schedule() {

	FTS3_COMMON_LOGGER_NEWLOG(INFO) << "FileTransferScheduler::schedule()" << commit;

	FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Checking if there are free outbound credits for the source SE." << commit;
	int freeCredits = getFreeCredits(OUTBOUND, VO, SE_TYPE, srcSeName, voName);
	if (getFreeCredits(OUTBOUND, VO, SE_TYPE, srcSeName, voName) <= 0) {
		return false;
	}

	FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Checking if there are free inbound credits for the destination SE." << commit;
	freeCredits = getFreeCredits(INBOUND, VO, SE_TYPE, destSeName, voName);
	if (getFreeCredits(INBOUND, VO, SE_TYPE, destSeName, voName) <= 0) {
		return false;
	}

	FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Checking if there are free outbound pair-credits for the source SE." << commit;
	freeCredits = getFreeCredits(OUTBOUND, PAIR, SE_TYPE, srcSeName, destSeName);
	if (getFreeCredits(OUTBOUND, PAIR, SE_TYPE, srcSeName, destSeName) <= 0) {
		return false;
	}

	FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Checking if there are free inbound pair-credits for the destination SE." << commit;
	freeCredits = getFreeCredits(INBOUND, PAIR, SE_TYPE, destSeName, srcSeName);
	if (getFreeCredits(INBOUND, PAIR, SE_TYPE, destSeName, srcSeName) <= 0) {
		return false;
	}

	FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Checking if there are free outbound credits for the source GROUP." << commit;
	freeCredits = getFreeCredits(OUTBOUND, VO, GROUP_TYPE, srcGroupName, voName);
	if (getFreeCredits(OUTBOUND, VO, GROUP_TYPE, srcGroupName, voName) <= 0) {
		return false;
	}

	FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Checking if there are free inbound credits for the destination GROUP." << commit;
	freeCredits = getFreeCredits(INBOUND, VO, GROUP_TYPE, destGroupName, voName);
	if (getFreeCredits(INBOUND, VO, GROUP_TYPE, destGroupName, voName) <= 0) {
		return false;
	}

	// No group pairs !!! :)

//	FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Checking if there are free outbound pair-credits for the source SITE." << commit;
//	if (getFreeCredits(OUTBOUND, PAIR, GROUP_TYPE, srcSiteName, destSiteName) <= 0) {
//		return false;
//	}
//
//	FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Checking if there are free inbound pair-credits for the destination SITE." << commit;
//	if (getFreeCredits(INBOUND, PAIR, GROUP_TYPE, destSiteName, srcSiteName) <= 0) {
//		return false;
//	}

	// update file state to READY
	int updated = db->updateFileStatus(file, JobStatusHandler::FTS3_STATUS_READY);
	if(updated == 0)
		return false;

	return true;
}

// SE only TODO
void FileTransferScheduler::initVosInQueue(vector<TransferFiles*> files) {

	vector<TransferFiles*>::iterator it;
	for (it = files.begin(); it < files.begin(); it++) {
		TransferFiles* tmp = *it;

		re.set_expression(seNameRegex);
		regex_match(tmp->SOURCE_SURL, what, re, match_extra);
		string srcSeName = what[SE_NAME_REGEX_INDEX];
		regex_match(tmp->DEST_SURL, what, re, match_extra);
		string destSeName = what[SE_NAME_REGEX_INDEX];

		if (srcSeName == this->srcSeName) {
			vosInQueueOut.insert(tmp->VO_NAME);
		}

		if (destSeName == this->destSeName) {
			vosInQueueIn.insert(tmp->VO_NAME);
		}
	}
}

// SE only TODO
int FileTransferScheduler::resolveSharedCredits(const string type, string name, Share share, IO io) {

	// TODO assumption PAIR configuration don't support shared policy

	vector<SeAndConfig*> seAndConfig;
	// determine if there is public shared configuration for the SE
	db->getAllShareAndConfigWithCritiria (
			seAndConfig,
			name,
			publicShare(),
			type,
			getSharedValue
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
					voShare(*v_it),
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
			getSharedValue
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
		re.set_expression(shareValueRegex);
		regex_match(share_id, what, re, match_extra);

		// vo or public
		string type = what[1];
		// vo name or empty if its public share
		string vo = what[3];

		// share value
		string share_value = cfg->SHARE_VALUE;
		// parse the share_value string
		re.set_expression(shareValueRegex);
		regex_match(share_value, what, re, match_extra);

		int value;
		switch (io) {
		case INBOUND:
			value = lexical_cast<int>(what[INBOUND_REGEX_INDEX]);

			break;
		case OUTBOUND:
			value = lexical_cast<int>(what[OUTBOUND_REGEX_INDEX]);
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
		} else if (&vosInQueue .find(vo) != &vosInQueue .end()) {
			// it is vo-share and the VO is not amongst VOs that have pending files
			sumInQueue += value;
		}
	}

	return credits / sumInQueue * (sumAll/*TODO - allSharedCreditsInUse*/);
}


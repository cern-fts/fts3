/*
 * FileTransferScheduler.cpp
 *
 *  Created on: May 7, 2012
 *      Author: simonm
 */


#include <boost/lexical_cast.hpp>

#include "FileTransferScheduler.h"
#include "db/generic/SingleDbInstance.h"

#include "common/logger.h"
#include "common/JobStatusHandler.h"


using namespace db;

const string FileTransferScheduler::SE_TYPE = "se";
const string FileTransferScheduler::SITE_TYPE = "site";

const string FileTransferScheduler::shareValueRegex = "\"in\":(\\d+),\"out\":(\\d+),\"policy\":\"(\\w+)\"";
const string FileTransferScheduler::seNameRegex = "(.+://.+:\\d+)/.+";

FileTransferScheduler::FileTransferScheduler(TransferFiles* file) {

	this->file = file;

	// prepare input for se part
	re.set_expression(seNameRegex);
	regex_match(file->SOURCE_SURL, what, re, match_extra);
	srcSeName = what[SE_NAME_REGEX_INDEX];

	regex_match(file->DEST_SURL, what, re, match_extra);
	destSeName = what[SE_NAME_REGEX_INDEX];

	voName = file->VO_NAME;

	// prepare input for site part
	Se* srcSe = 0;
	DBSingleton::instance().getDBObjectInstance()->getSe(srcSe, srcSeName);
	if (srcSe) {
		srcSiteName = srcSe->SITE;
		delete srcSe;
	}

	Se* destSe = 0;
	DBSingleton::instance().getDBObjectInstance()->getSe(destSe, destSeName);
	if(destSe) {
		destSiteName = destSe->SITE;
		delete destSe;
	}
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
	vector<SeAndConfig*>::iterator it;

	// query the DB
	DBSingleton::instance().getDBObjectInstance()->getAllSeAndConfigWithCritiria(
			seAndConfig,
			name,
			shareId,
			type,
			""
		);

	// there can be only one entry in the vector
	if (!seAndConfig.empty()) {

		// parse the string
		re.set_expression(shareValueRegex);
		regex_match(seAndConfig[0]->SHARE_VALUE, what, re, match_extra);

		// set the values
		ret.inbound = lexical_cast<int>(what[INBOUND_REGEX_INDEX]);
		ret.outbound = lexical_cast<int>(what[OUTBOUND_REGEX_INDEX]);
		ret.set = true;

		FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Inbound: " << ret.inbound << ", outbound: " << ret.outbound << commit;

		// free the memory
		for (it = seAndConfig.begin(); it < seAndConfig.end(); it++) {
			delete *it;
		}
	} else {
		FTS3_COMMON_LOGGER_NEWLOG(INFO) << "No configuration found." << commit;
	}

	return ret;
}

int FileTransferScheduler::getCreditsInUse(IO io, Share share, const string type) {

	FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Looking for credits in use for: "
			<< (io == INBOUND ? "inbound" : "outbound") << ", "
			<< (share == PAIR ? "pair" : share == VO ? "vo" : "public") << "share, "
			<< (type == SE_TYPE ? "se" : "site")
			<< commit;

	int creditsInUse;

	string srcSeName, destSeName, srcSiteName, destSiteName, voName;

	switch (share) {
	// if we are interested in voshare set vo's name
	case VO:
		voName = this->voName;
		break;
	// if we are interested in pairshare set both src and dest
	case PAIR:
		srcSeName = this->srcSeName;
		srcSiteName = this->srcSiteName;
		destSeName = this->destSeName;
		destSiteName = this->destSiteName;
		break;
	// otherwise (e.g. public share) do nothing
	default: break;
	}

	switch (io) {
	// if we are interested in outbound set the src
	case OUTBOUND:
		srcSeName = this->srcSeName;
		srcSiteName = this->srcSiteName;
		break;
	// if we are interested in inbound set the dest
	case INBOUND:
		destSeName = this->destSeName;
		destSiteName = this->destSiteName;
		break;
	}

	// query for the credits in DB
	if (type == SE_TYPE) {
		// we are looking for SE credits
		DBSingleton::instance().getDBObjectInstance()->getSeCreditsInUse(
				creditsInUse,
				srcSeName,
				destSeName,
				voName
			);

	} else {
		// we are looking for SITE credits
		DBSingleton::instance().getDBObjectInstance()->getSeCreditsInUse(
				creditsInUse,
				srcSeName,
				destSeName,
				voName
			);
	}

	FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Credits in use: " << creditsInUse << commit;

	return creditsInUse;
}

int FileTransferScheduler::getFreeCredits(IO io, Share share, const string type, string name, string param) {

	FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Looking for: "
			<< (io == OUTBOUND ? "outbound" : "inbound") << " credits, "
			<< (share == VO ? "vo" : "pair") << "share: " << param << ", "
			<< (type == SE_TYPE ? "se" : "site") << ": " << name
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
					<< (type == SE_TYPE ? "se" : "site") << "-pair configuration."
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
			<< (type == SE_TYPE ? "se" : "site") << ": " << name
			<< commit;
	// look for public share
	cfg = getValue(type, name, publicShare());

	// if a cfg has been found, and return the difference
	if (cfg) return cfg[io] - creditsInUse;

	if (type == SITE_TYPE) {
		FTS3_COMMON_LOGGER_NEWLOG(INFO) << "There are no site settings in the DB!" << commit;
		return FREE_CREDITS;
	}

	FTS3_COMMON_LOGGER_NEWLOG(INFO) << "The default value is used." << commit;
	// otherwise return the difference for the default value
	return DEFAULT_VALUE - creditsInUse;
}

bool FileTransferScheduler::schedule() {

	FTS3_COMMON_LOGGER_NEWLOG(INFO) << "FileTransferScheduler::schedule()" << commit;

	FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Checking if there are free outbound credits for the source SE." << commit;
	if (getFreeCredits(OUTBOUND, VO, SE_TYPE, srcSeName, voName) <= 0) {
		return false;
	}

	FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Checking if there are free inbound credits for the destination SE." << commit;
	if (getFreeCredits(INBOUND, VO, SE_TYPE, destSeName, voName) <= 0) {
		return false;
	}

	FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Checking if there are free outbound pair-credits for the source SE." << commit;
	if (getFreeCredits(OUTBOUND, PAIR, SE_TYPE, srcSeName, destSeName) <= 0) {
		return false;
	}

	FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Checking if there are free inbound pair-credits for the destination SE." << commit;
	if (getFreeCredits(INBOUND, PAIR, SE_TYPE, destSeName, srcSeName) <= 0) {
		return false;
	}

	FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Checking if there are free outbound credits for the source SITE." << commit;
	if (getFreeCredits(OUTBOUND, VO, SITE_TYPE, srcSiteName, voName) <= 0) {
		return false;
	}

	FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Checking if there are free inbound credits for the destination SITE." << commit;
	if (getFreeCredits(INBOUND, VO, SITE_TYPE, srcSiteName, destSiteName) <= 0) {
		return false;
	}

	FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Checking if there are free outbound pair-credits for the source SITE." << commit;
	if (getFreeCredits(OUTBOUND, PAIR, SITE_TYPE, srcSiteName, destSiteName) <= 0) {
		return false;
	}

	FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Checking if there are free inbound pair-credits for the destination SITE." << commit;
	if (getFreeCredits(INBOUND, PAIR, SITE_TYPE, destSiteName, srcSiteName) <= 0) {
		return false;
	}

	// update file state to READY
	DBSingleton::instance().getDBObjectInstance()->updateFileStatus(file, JobStatusHandler::FTS3_STATUS_READY);

	return true;
}

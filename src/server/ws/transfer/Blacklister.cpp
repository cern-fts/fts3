/*
 * Blacklister.cpp
 *
 *  Created on: Mar 27, 2013
 *      Author: simonm
 */

#include "Blacklister.h"

#include "common/logger.h"

#include "ws/CGsiAdapter.h"

namespace fts3 {
namespace ws {

using namespace fts3::common;

Blacklister::Blacklister(soap* ctx, string name, string status, int timeout, bool blk) :
		name(name),
		status(status),
		timeout(timeout),
		blk(blk),
		db(DBSingleton::instance().getDBObjectInstance()) {

	CGsiAdapter cgsi(ctx);
	adminDn = cgsi.getClientDn();
}

Blacklister::Blacklister(soap* ctx, string name, string vo, string status, int timeout, bool blk) :
		name(name),
		vo(vo),
		status(status),
		timeout(timeout),
		blk(blk),
		db(DBSingleton::instance().getDBObjectInstance()) {

	CGsiAdapter cgsi(ctx);
	adminDn = cgsi.getClientDn();
}

Blacklister::~Blacklister() {

}

void Blacklister::executeRequest() {

	// the vo parameter is given only for SE blacklisting
	if (vo.is_initialized()) {

		handleSeBlacklisting();

	} else {

		handleDnBlacklisting();
	}
}

void Blacklister::handleSeBlacklisting() {

	// audit the operation
	string cmd = "fts-set-blacklist se " + name + (blk ? " on" : " off");
	db->auditConfiguration(adminDn, cmd, "blacklist");

	if (blk) {
		db->blacklistSe(
				name, *vo, status, timeout, string(), adminDn
			);
		// log it
		FTS3_COMMON_LOGGER_NEWLOG (INFO) << "User: " << adminDn << " had blacklisted the SE: " << name << commit;

		// TODO CANCEL transfer

	} else {
		db->unblacklistSe(name);
		// log it
		FTS3_COMMON_LOGGER_NEWLOG (INFO) << "User: " << adminDn << " had unblacklisted the SE: " << name << commit;
	}
}

void Blacklister::handleDnBlacklisting() {

	// audit the operation
	string cmd = "fts-set-blacklist dn " + name + (blk ? " on" : " off");
	db->auditConfiguration(adminDn, cmd, "blacklist");

	if (blk) {
		db->blacklistDn(name, string(), adminDn);
		// log it
		FTS3_COMMON_LOGGER_NEWLOG (INFO) << "User: " << name << " had blacklisted the DN: " << adminDn << commit;
	} else {
		db->unblacklistDn(name);
		// log it
		FTS3_COMMON_LOGGER_NEWLOG (INFO) << "User: " << adminDn << " had unblacklisted the DN: " << name << commit;
	}
}

} /* namespace ws */
} /* namespace fts3 */

/*
 * DelegationoOrchestrator.cpp
 *
 *  Created on: Oct 29, 2012
 *      Author: simonm
 */

#include "DelegationoOrchestrator.h"

namespace fts3 {
namespace ws {

bool DelegationoOrchestrator::orchestrate(string delegationId, string& key) {
	mutex::scoped_lock lock(m);

	if (pkeys.find(delegationId) != pkeys.end()) {
		key = pkeys[delegationId];
		return true;
	}

	pkeys[delegationId] = key;
	return false;
}

bool DelegationoOrchestrator::check (string delegationId) {
	return pkeys.find(delegationId) == pkeys.end();
}

void DelegationoOrchestrator::remove (string delegationId) {
	mutex::scoped_lock lock(m);
	pkeys.erase(delegationId);
}

} /* namespace ws */
} /* namespace fts3 */

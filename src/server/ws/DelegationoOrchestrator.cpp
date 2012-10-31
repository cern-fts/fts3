/*
 * DelegationoOrchestrator.cpp
 *
 *  Created on: Oct 29, 2012
 *      Author: simonm
 */

#include "DelegationoOrchestrator.h"

namespace fts3 {
namespace ws {

void DelegationoOrchestrator::put (string delegationId, string key) {
	mutex::scoped_lock lock(m);
	pkeys[delegationId] = key;
}

void DelegationoOrchestrator::remove (string delegationId) {
	mutex::scoped_lock lock(m);
	pkeys.erase(delegationId);
}

string DelegationoOrchestrator::get(string delegationId) {
	mutex::scoped_lock lock(m);
	return pkeys[delegationId];
}

} /* namespace ws */
} /* namespace fts3 */

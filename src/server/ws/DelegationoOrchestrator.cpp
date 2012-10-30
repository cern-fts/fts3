/*
 * DelegationoOrchestrator.cpp
 *
 *  Created on: Oct 29, 2012
 *      Author: simonm
 */

#include "DelegationoOrchestrator.h"

namespace fts3 {
namespace ws {


DelegationoOrchestrator::~DelegationoOrchestrator() {

	// notify all pending threads
	map<string, shared_ptr<condition_variable> >::iterator it;
	for (it = access.begin(); it != access.end(); it++) {
		it->second->notify_all();
	}
}

bool DelegationoOrchestrator::wait(string delegationId, long timeout) {
	mutex::scoped_lock lock(m);
	map<string, shared_ptr<condition_variable> >::iterator it = access.find(delegationId);
	if (it == access.end()) {
		access[delegationId] = shared_ptr<condition_variable>(new condition_variable());
		return false;
	}

	shared_ptr<condition_variable> cond = it->second;
	system_time t = get_system_time() + posix_time::milliseconds(timeout);
	return cond->timed_wait(lock, t);
}


void DelegationoOrchestrator::notify(string delegationId) {
	mutex::scoped_lock lock(m);

	map<string, shared_ptr<condition_variable> >::iterator it = access.find(delegationId);
	it->second->notify_all();
	access.erase(delegationId);
}

} /* namespace ws */
} /* namespace fts3 */

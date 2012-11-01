/*
 * DelegationoOrchestrator.h
 *
 *  Created on: Oct 29, 2012
 *      Author: simonm
 */

#ifndef DELEGATIONOORCHESTRATOR_H_
#define DELEGATIONOORCHESTRATOR_H_

#include "common/ThreadSafeInstanceHolder.h"

#include <boost/thread.hpp>

#include <string>
#include <map>

namespace fts3 {
namespace ws {

using namespace std;
using namespace boost;
using namespace fts3::common;

/**
 * A DelegationRequestCache orchestrates the proces of proxy certificate delegation
 * in case if two or more clients with the same DN and VO are trying to renew a proxy
 *
 * DelegationOrchestrator is a singleton
 *
 */
class DelegationRequestCache: public ThreadSafeInstanceHolder<DelegationRequestCache>  {

	friend class ThreadSafeInstanceHolder<DelegationRequestCache>;

public:

	/**
	 * The destructor
	 */
	virtual ~DelegationRequestCache(){};

	/**
	 * Adds new public key request to the cache
	 *
	 * @param delegationId - delegation ID (unique for a given VO and DN)
	 * @param rqst - public key request
	 */
	void put(string delegationId, string rqst) {
		pkeys[delegationId] = rqst;
	}

	/**
	 * Removes a public key request from the cache
	 *
	 * @param delegationId - delegation ID (unique for a given VO and DN)
	 */
	void remove(string delegationId) {
		pkeys.erase(delegationId);
	}

	/**
	 * Gets the public key request from the cache
	 *
	 * @param delegationId - delegation ID (unique for a given VO and DN)
	 */
	string get(string delegationId) {
		return pkeys[delegationId];
	}

	/**
	 * Checks if the public key request is in the cache
	 *
	 * @param delegationId - delegation ID (unique for a given VO and DN)
	 */
	bool check(string delegationId) {
		return pkeys.find(delegationId) != pkeys.end();
	}

	/**
	 * Mutex cast operator, should be used with 'scoped_lock'
	 * to lock the cache before any operation is executed
	 */
	operator mutex&() {
		return m;
	}

private:
	/**
	 * Default constructor
	 *
	 * Private, should not be used
	 */
	DelegationRequestCache() {};

	/**
	 * Copying constructor
	 *
	 * Private, should not be used
	 */
	DelegationRequestCache(DelegationRequestCache const&);

	/**
	 * Assignment operator
	 *
	 * Private, should not be used
	 */
	DelegationRequestCache& operator=(DelegationRequestCache const&);

	/**
	 * the map holding a public key to each proxy certificate that is being delegated at the moment
	 */
	map<string, string> pkeys;

	mutex m;
};

} /* namespace ws */
} /* namespace fts3 */
#endif /* DELEGATIONOORCHESTRATOR_H_ */

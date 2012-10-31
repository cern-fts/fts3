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
#include <boost/shared_ptr.hpp>

#include <string>
#include <map>

namespace fts3 {
namespace ws {

using namespace std;
using namespace boost;
using namespace fts3::common;

class DelegationoOrchestrator: public ThreadSafeInstanceHolder<DelegationoOrchestrator>  {

	friend class ThreadSafeInstanceHolder<DelegationoOrchestrator>;

public:
	virtual ~DelegationoOrchestrator(){};

	bool orchestrate(string delegationId, string& key);
	bool check (string delegationId);
	void remove (string delegationId);

private:
	/**
	 * Default constructor
	 *
	 * Private, should not be used
	 */
	DelegationoOrchestrator() {};

	/**
	 * Copying constructor
	 *
	 * Private, should not be used
	 */
	DelegationoOrchestrator(DelegationoOrchestrator const&);

	/**
	 * Assignment operator
	 *
	 * Private, should not be used
	 */
	DelegationoOrchestrator& operator=(DelegationoOrchestrator const&);

	/**
	 * the map holding conditions corresponding to each delegation
	 */
	map<string, string> pkeys;

	mutex m;
};

} /* namespace ws */
} /* namespace fts3 */
#endif /* DELEGATIONOORCHESTRATOR_H_ */

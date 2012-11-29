/*
 * TransferJobApprover.h
 *
 *  Created on: Nov 6, 2012
 *      Author: simonm
 */

#ifndef TRANSFERJOBAPPROVER_H_
#define TRANSFERJOBAPPROVER_H_

#include <string>

namespace fts3 {
namespace ws {

using namespace std;

class TransferJobApprover {
public:
	TransferJobApprover();
	virtual ~TransferJobApprover();

	void approve();

	bool doOsgCheck(string se);
	bool doBdiiCheck(string se);
};

} /* namespace ws */
} /* namespace fts3 */
#endif /* TRANSFERJOBAPPROVER_H_ */

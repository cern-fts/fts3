/*
 * SubmitTransferCli.h
 *
 *  Created on: Feb 2, 2012
 *      Author: simonm
 */

#ifndef SUBMITTRANSFERCLI_H_
#define SUBMITTRANSFERCLI_H_

#include "CliBase.h"
#include "ftsStub.h"
#include <list>

class SubmitTransferCli : public CliBase {
public:
	SubmitTransferCli();
	virtual ~SubmitTransferCli();

	transfer__TransferParams* getParams(soap* soap);
	bool performChecks();
	virtual void initCli(int ac, char* av[]);
	string getPassword();

private:
	options_description specific;
	options_description cfg_desc;

	string cfg_file;
	string password;

	bool delegate;
};

#endif /* SUBMITTRANSFERCLI_H_ */

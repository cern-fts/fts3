/*
 * ListTransferCli.h
 *
 *  Created on: Mar 1, 2012
 *      Author: simonm
 */

#ifndef LISTTRANSFERCLI_H_
#define LISTTRANSFERCLI_H_

#include "CliBase.h"
#include "GsoapStubs.h"

namespace fts { namespace cli {

class ListTransferCli : public CliBase {
public:

	/**
	 * Default constructor.
	 *
	 * Creates the transfer-list specific command line options. State is
	 * market as both: hidden and positional
	 */
	ListTransferCli();

	/**
	 * Destructor.
	 */
	virtual ~ListTransferCli();

	/**
	 * Gives the instruction how to use the command line tool.
	 *
	 * @return a string with instruction on how to use the tool
	 */
	string getUsageString();

	/**
	 * Gets the VO name, specified by the user.
	 *
	 * @return VO name
	 */
	string getVoname();

	/**
	 * Gets the user DN, specified by the user.
	 *
	 * @return user DN
	 */
	string getUserdn();

	/**
	 * Check if server supports -u and -o options.
	 *
	 * @return true if all used options are supported by the FTS3 service
	 */
	bool checkIfFeaturesSupported();

	/**
	 * Gets a pointer to fts__ArrayOf_USCOREsoapenc_USCOREstring object.
	 * The object contains set of statuses of interest. This are the statuses
	 * that the user passed using command line options. If no statuses were
	 * passed, the set is determined depending on the version of the FTS3 service.
	 * The object is created using gSOAP memory-allocation utility, and will
	 * be garbage collected. If there is a need to delete it manually gSOAP dedicated
	 * functions should be used (in particular 'soap_unlink'!)
	 *
	 * @param soap - soap object corresponding to FTS3 service
	 *
	 * @return  fts__ArrayOf_USCOREsoapenc_USCOREstring
	 */
	fts__ArrayOf_USCOREsoapenc_USCOREstring* getStatusArray(soap* soap);

private:

	/**
	 * command line options specific for fts3-transfer-list
	 */
	options_description specific;

	/**
	 * hidden command line options (not printed in help)
	 */
	options_description hidden;

	string FTS3_STATUS_SUBMITTED;
	string FTS3_STATUS_PENDING;
	string FTS3_STATUS_READY;
	string FTS3_STATUS_ACTIVE;
};

}
}

#endif /* LISTTRANSFERCLI_H_ */

/*
 * BlacklistCli.h
 *
 *  Created on: Nov 7, 2012
 *      Author: simonm
 */

#ifndef BLACKLISTCLI_H_
#define BLACKLISTCLI_H_

#include "CliBase.h"

#include <string>

namespace fts3 {
namespace cli {

using namespace std;

class BlacklistCli : public CliBase {

public:

	static const string ON;
	static const string OFF;

	static const string SE;
	static const string DN;

	/**
	 * Default Constructor.
	 *
	 * Creates the debud-set specific command line option: on/off.
	 */
	BlacklistCli();

	/**
	 * Destructor.
	 */
	virtual ~BlacklistCli();

	/**
	 * Validates command line options.
	 *	Checks that the debug mode was set correctly.
	 *
	 * @return GSoapContexAdapter instance, or null if all activities
	 * 				requested using program options have been done.
	 */
	virtual optional<GSoapContextAdapter&> validate(bool init = true);

	/**
	 * Gives the instruction how to use the command line tool.
	 *
	 * @return a string with instruction on how to use the tool
	 */
	string getUsageString(string tool);

	/**
	 * Gets the debug mode.
	 *
	 * @return true is the debug mode is on, false if the debud mode is off
	 */
	bool getBlkMode() {
		return mode == ON;
	}

	string getSubjectName() {
		return subject;
	}

	string getSubjectType() {
		return type;
	}

private:

	/// blacklist mode, either ON or OFF
	string mode;

	/// the DN or SE that is the subject of blacklisting
	string subject;

	/// type of the subject, either SE or DN
	string type;
};

} /* namespace cli */
} /* namespace fts3 */
#endif /* BLACKLISTCLI_H_ */

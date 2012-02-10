/*
 * CliManager.h
 *
 *  Created on: Feb 7, 2012
 *      Author: simonm
 */

#ifndef CLIMANAGER_H_
#define CLIMANAGER_H_

#include <string>
using namespace std;

class CliManager {
public:

	static CliManager* getInstance();
	virtual ~CliManager();

	void setInterface(string interface);
	string getPassword();

	string FTS3_PARAM_GRIDFTP;
	string FTS3_PARAM_MYPROXY;
	string FTS3_PARAM_DELEGATIONID;
	string FTS3_PARAM_SPACETOKEN;
	string FTS3_PARAM_SPACETOKEN_SOURCE;
	string FTS3_PARAM_COPY_PIN_LIFETIME;
	string FTS3_PARAM_LAN_CONNECTION;
	string FTS3_PARAM_FAIL_NEARLINE;
	string FTS3_PARAM_OVERWRITEFLAG;
	string FTS3_PARAM_CHECKSUM_METHOD;

	string FTS3_CA_SD_TYPE;
	string FTS3_SD_ENV;
	string FTS3_SD_TYPE;
	string FTS3_IFC_VERSION;
	string FTS3_INTERFACE_VERSION;

	int isChecksumSupported();
	int isDelegationSupported();
	int isRolesOfSupported();
	int isSetTCPBufferSupported();
	int isExtendedChannelListSupported();
	int isChannelMessageSupported();
	int isUserVoRestrictListingSupported();
	int isVersion330StatesSupported();
	int isItVersion330();
	int isItVersion340();
	int isItVersion350();
	int isItVersion370();

private:
	CliManager();
	CliManager (CliManager const&);
	CliManager& operator=(CliManager const&);

	static CliManager* manager;
	long major;
	long minor;
	long patch;
};

#endif /* CLIMANAGER_H_ */

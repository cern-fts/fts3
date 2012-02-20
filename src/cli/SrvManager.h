/*
 *	Copyright notice:
 *	Copyright © Members of the EMI Collaboration, 2010.
 *
 *	See www.eu-emi.eu for details on the copyright holders
 *
 *	Licensed under the Apache License, Version 2.0 (the "License");
 *	you may not use this file except in compliance with the License.
 *	You may obtain a copy of the License at
 *
 *		http://www.apache.org/licenses/LICENSE-2.0
 *
 *	Unless required by applicable law or agreed to in writing, software
 *	distributed under the License is distributed on an "AS IS" BASIS,
 *	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *	See the License for the specific language governing permissions and
 *	limitations under the License.
 */

/*
 * SrvManager.h
 *
 *  Created on: Feb 7, 2012
 *      Author: simonm
 */

#ifndef SRVMANAGER_H_
#define SRVMANAGER_H_

#include <string>
#include "GsoapStubs.h"

using namespace std;


namespace fts { namespace cli {

class SrvManager {
public:

	enum TransferStateEnum {
	    FTS3_TRANSFER_FAILED = -6,
	    FTS3_TRANSFER_CATALOGFAILED = -5,
	    FTS3_TRANSFER_FINISHED_DIRTY = -4,
	    FTS3_TRANSFER_UNKNOWN = -3,
	    FTS3_TRANSFER_CANCELED = -2,
	    FTS3_TRANSFER_TRANSFERFAILED = -1,
	    FTS3_TRANSFER_FINISHED = 0,
	    FTS3_TRANSFER_SUBMITTED,
	    FTS3_TRANSFER_PENDING,
	    FTS3_TRANSFER_ACTIVE,
	    FTS3_TRANSFER_CANCELING,
	    FTS3_TRANSFER_WAITING,
	    FTS3_TRANSFER_HOLD,
	    FTS3_TRANSFER_DONE,
	    FTS3_TRANSFER_READY,
	    FTS3_TRANSFER_DONEWITHERRORS,
	    FTS3_TRANSFER_FINISHING,
	    FTS3_TRANSFER_AWAITING_PRESTAGE,
	    FTS3_TRANSFER_PRESTAGING,
	    FTS3_TRANSFER_WAITING_PRESTAGE,
	    FTS3_TRANSFER_WAITING_CATALOG_RESOLUTION,
	    FTS3_TRANSFER_WAITING_CATALOG_REGISTRATION
	};

	static const int statusCount = 21;

	struct TransferState {
		string name;
		TransferStateEnum id;
	};

	static SrvManager* getInstance();
	virtual ~SrvManager();

	bool initSoap(soap* soap, string endpoint);

	bool init(FileTransferSoapBindingProxy& service);
	void setInterfaceVersion(string interface);
	string getPassword();
	long isCertValid (string serviceLocation);
	bool isTransferReady(string status);
	void delegateProxyCert(string service);

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

	string getInterface() {
		return interface;
	}

	string getVersion() {
		return version;
	}

	string getSchema() {
		return schema;
	}

	string getMetadata() {
		return metadata;
	}

private:
	SrvManager();
	SrvManager (SrvManager const&);
	SrvManager& operator=(SrvManager const&);

	static SrvManager* manager;

	long major;
	long minor;
	long patch;

	string interface;
	string version;
	string schema;
	string metadata;

	static TransferState statuses[];
};

}
}

#endif /* SRVMANAGER_H_ */

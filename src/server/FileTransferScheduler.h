/*
 *	Copyright notice:
 *	Copyright � Members of the EMI Collaboration, 2010.
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
 * GSoapContextAdapter.h
 *
 * FileTransferScheduler.h
 *
 *  Created on: May 7, 2012
 *      Author: Micha\u0142 Simon
 */

#ifndef FILETRANSFERSCHEDULER_H_
#define FILETRANSFERSCHEDULER_H_

#include <vector>
#include <string>
#include <set>
#include <map>

#include "boost/tuple/tuple.hpp"
#include <boost/optional.hpp>

#include "db/generic/SingleDbInstance.h"

using namespace std;
using namespace boost;

/**
 * The FileTransferScheduler class is responsible for scheduling transfer files
 * A file may either scheduled for transferring, in this case its status is changed
 * from Submitted to Ready, or for waiting until free credits are available for
 * this type of transfer.
 */
class FileTransferScheduler {
public:

	/**
	 * Constructor
	 *
	 * @param file - the file for which the scheduling decision has to be taken
	 */
	FileTransferScheduler(TransferFiles* file, vector<TransferFiles*> otherFiles = vector<TransferFiles*>());

	/**
	 * Destructor
	 */
	virtual ~FileTransferScheduler();

	/**
	 * Makes the scheduling decision for the file that has been used to create the
	 * object, changes the file status to ready is there are free credits
	 *
	 * @return returns true if file status has been changed to Ready, false otherwise
	 */
	bool schedule(bool optimize, bool manualConfig);

private:

	/**
	 * IO enumeration
	 */
	enum IO {
		/// INBOUND is used to emphasize that we are interested in inbound credit
		INBOUND,
		/// OUTBOUND is used to emphasize that we are interested in outbound credit
		OUTBOUND
	};

	/**
	 * Share enumeration
	 */
	enum Share {
		/// VO is used to emphasize that we are interested in voshare credits
		VO,
		/// VO is used to emphasize that we are interested in publicshare credits
		PUBLIC,
		/// VO is used to emphasize that we are interested in pairshare credits
		PAIR
	};

	/**
	 * Gets credits in use
	 *
	 * @param io - OUTBOUND if we are interested in the outbound credits of the source SE,
	 * 				or INBOUND if we are interested in the inbound credits of the destination SE
	 * @param share - specifies the share type we are interested in (PUBLIC, VO or PAIR)
	 * @param type - specifies the credit type we are interested in (SE or GROUP)
	 *
	 * @return the number of credits in use
	 */
	int getCreditsInUse(IO io, Share share, const string type);

	/**
	 * Gets free credits
	 *
	 * @param io - OUTBOUND if we are interested in the outbound credits of the source SE,
	 * 				or INBOUND if we are interested in the inbound credits of the destination SE
	 * @param share - specifies the share type we are interested in (PUBLIC, VO or PAIR)
	 * @param type - specifies the credit type we are interested in (SE or GROUP)
	 *
	 * @return the number of free credits
	 */
	int getFreeCredits(IO io, Share share, const string type, string name, string param);

	/**
	 * TODO
	 */
	int resolveSharedCredits(const string type, string name, Share share, IO io);

	/**
	 * Gets the configuration of the requested type
	 *
	 * @param type - type of the entity, either 'se' or 'group'
	 * @param name - SE or SE group name
	 * @param shareId - the share ID of the configuration, should be obtained
	 * 					by using either voShare, publicShare or pairShare
	 */
	optional< tuple<int, int, string> > getValue (const string type, string name, string shareId);

	/**
	 * Gets either inbound or outbound credits
	 *
	 * @param val - tuple containing cfg value
	 * @param io - specifies if we are interested in outbound or inbound credits
	 *
	 * @return the number of outbound/inbound credits
	 */
	int getCredits(tuple<int, int, string> val, IO io) {
		return 0;
	}

	/**
	 * Initializes the seNamesToPendingVos map using the a list of pending transfers
	 *
	 * @param files - the list of pending files
	 */
	void initVosInQueue(vector<TransferFiles*> files);

	/// pointer to the file that has to be scheduled
	TransferFiles* file;
	/// name of the source SE
	string srcSeName;
	/// name of the destination SE
	string destSeName;
	/// the VO name of the submitter
	string voName;
	/// name of the source SE group
	string srcGroupName;
	/// name of the destination SE group
	string destGroupName;

	/// the default number of outbounds and inbounds credits
	/// used if the SE has not been configured
	static const int DEFAULT_VALUE = 50;
	/// a constant used to emphasize that there are free credits
	static const int FREE_CREDITS = 1;

	// DB singleton instance
	GenericDbIfce* db;

	/// maps a SE name to VO names who have pending inbound transfers in the queue
	set<string> vosInQueueIn;
	/// maps a SE name to VO names who have pending outbound transfers in the queue
	set<string> vosInQueueOut;
};

#endif /* FILETRANSFERSCHEDULER_H_ */

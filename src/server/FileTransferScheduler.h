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
 * GSoapContextAdapter.h
 *
 * FileTransferScheduler.h
 *
 *  Created on: May 7, 2012
 *      Author: Michał Simon
 */

#ifndef FILETRANSFERSCHEDULER_H_
#define FILETRANSFERSCHEDULER_H_

#include <vector>
#include <string>

#include <boost/regex.hpp>

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
	FileTransferScheduler(TransferFiles* file);

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
	bool schedule();

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
	 * The Config structure has the information about a SE configuration.
	 * 	If the set field is set to true the data have been read from DB.
	 */
	struct Config {
		/// the inbound credits
		int inbound;
		/// the outbound credits
		int outbound;
		/// true if the date have been read from DB, false means that the data are junk
		bool set;

		/**
		 * Default constructor. Sets set to false.
		 */
		Config() {
			set = false;
		}

		/**
		 * bool casting operator
		 *
		 * @return the value of set field
		 */
		operator bool() const {
			return set;
		}

		/**
		 * indexing operator
		 *
		 * 	@return the number of inbound credits if the argument was INBOUND,
		 * 			the number of outbound credits if the argument was OUTBOUND,
		 * 			0 otherwise
		 */
		int operator[] (IO index) {

			switch(index) {
			case INBOUND: return inbound;
			case OUTBOUND: return outbound;
			default: return 0;
			}
		}

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
	 * Gets the configuration of the requested type
	 *
	 * @param type - type of the entity, either 'se' or 'group'
	 * @param name - SE or SE group name
	 * @param shareId - the share ID of the configuration, should be obtained
	 * 					by using either voShare, publicShare or pairShare
	 */
	Config getValue (const string type, string name, string shareId);

	/**
	 * Constructs the share ID of voshare type
	 *
	 * @return the voshare ID string for given vo name
	 */
	inline string voShare(string vo) {
		return "\"share_type\":\"vo\",\"share_id\":\"" + vo + "\"";
	};

	/**
	 * Constructs the share ID of publicshare type
	 *
	 * @return the publicshare ID string
	 */
	inline string publicShare() {
		return "\"share_type\":\"public\",\"share_id\":null";
	};

	/**
	 * Constructs the share ID of pairshare type
	 *
	 * @return  the pairshare ID string
	 */
	inline string pairShare(string pair) {
		return "\"share_type\":\"pair\",\"share_id\":\"" + pair + "\"";
	}

	/// regular expression object
	regex re;
	/// object containing strings that matched the regular expression
	smatch what;

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

	/// The index of the SE name group (NOT a SE group!!!) in the regular expression
	static const int SE_NAME_REGEX_INDEX = 1;
	/// The index of the inbound credits group in the regular expression
	static const int INBOUND_REGEX_INDEX = 1;
	/// The index of the outbound credits group in the regular expression
	static const int OUTBOUND_REGEX_INDEX = 2;

	/// a SE type string ('se')
	static const string SE_TYPE;
	/// a SE group type string ('group')
	static const string GROUP_TYPE;

	/// the regular expression used for retrieving the SE name from source or destination name
	static const string seNameRegex;
	/// the regular expression used for retrieving the inbound and outbound values
	/// from the JSON configuration stored in the DB
	static const string shareValueRegex;

	/// the default number of outbounds and inbounds credits
	/// used if the SE has not been configured
	static const int DEFAULT_VALUE = 50;
	/// a constant used to emphasize that there are free credits
	static const int FREE_CREDITS = 1;
};

#endif /* FILETRANSFERSCHEDULER_H_ */

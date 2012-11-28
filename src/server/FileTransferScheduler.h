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

	enum {
		SOURCE,
		DESTINATION,
		VO
	};

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
	bool schedule(bool optimize, bool manualConfig);

private:

	/// pointer to the file that has to be scheduled
	TransferFiles* file;
	/// name of the source SE
	string srcSeName;
	/// name of the destination SE
	string destSeName;


	// DB singleton instance
	GenericDbIfce* db;

	///
	static const regex fileUrlRegex;

	/**
	 *
	 */
	string fileUrlToSeName(string url);
};

#endif /* FILETRANSFERSCHEDULER_H_ */

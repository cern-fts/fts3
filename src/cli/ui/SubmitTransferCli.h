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
 *
 * SubmitTransferCli.h
 *
 *  Created on: Feb 2, 2012
 *      Author: Michal Simon
 */

#ifndef SUBMITTRANSFERCLI_H_
#define SUBMITTRANSFERCLI_H_

#include "CliBase.h"
#include "gsoap_proxy.h"
#include <vector>

using namespace std;
using namespace boost;


namespace fts3 { namespace cli {

/**
 * SubmitTransferCli is the command line utility used for the fts3-transfer-submit tool.
 *
 * In addition to the inherited functionalities from CliBase the SubmitTransferCli class provides:
 * 	 	- source (--source), the first positional parameter (passed without any switch option)
 * 	 		parameter is regarded as source
 * 	 	- destination (--destination), the second positional parameter is regarded as destination
 * 	 	- blocking (-b)
 * 	 	- bulk-job file (-f)
 * 	 	- grid ftp parameters (-g)
 * 	 	- interval (-i)
 * 	 	- MyProxy server (-m)
 * 	 	- password (-p)
 * 	 	- ID (-I)
 * 	 	- expire (-e)
 * 	 	- overwrite (-o)
 * 	 	- destination token (-t)
 * 	 	- source token (-S)
 * 	 	- compare checksum (-K)
 * 	 	- pin lifetime (--copy-pin-lifetime)
 * 	 	- LAN connection (--lan-connection)
 * 	 	- fail if nearline (--fail-nearline)
 *
 * @see CliBase
 */
class SubmitTransferCli : public CliBase {

	/**
	 * Single job element.
	 *
	 * Since there are two different classed that represent the job element
	 * (tns3__TransferJobElement, transfer__TransferJobElement2) this
	 * structure is used to store job element data until it is known which
	 * kind of job element should be used
	 *
	 */
	struct JobElement {
		string src; ///< source file
		string dest; ///< destination file
		string checksum; ///< the checksum

		static const int size = 3; ///< size of the JobElement (3x string)

		/**
		 * Subscript operator.
		 * Since we want to loop over the fields we cannot use boost::tuple.
		 *
		 * @return field corresponding to the given index, e.g. 0 - src.
		 */
		inline string& operator[] (const int index) {
			return ((string*)this)[index];
		}
	};

public:

	/**
	 * Default constructor.
	 *
	 * Initializes string fields corresponding to FTS3 parameters. Moreover, creates
	 * the transfer-submit specific command line options, source, destination and
	 * checksum are also market as positional options, checksum is a hidden option
	 * (not printed in help), other oprions are visible.
	 */
	SubmitTransferCli();

	/**
	 * Destructor
	 */
	virtual ~SubmitTransferCli();

	/**
	 * Initializes the object with command line options.
	 *
	 * In addition sets delegation flag.
	 *
	 * @param ac - argument count
	 * @param av - argument array
	 *
	 * @see CliBase::initCli(int, char*)
	 */
	virtual void parse(int ac, char* av[]);

	/**
	 * Validates command line options
	 * 1. Checks the endpoint
	 * 2. If -h or -V option were used respective informations are printed
	 * 3. GSoapContexAdapter is created, and info about server requested
	 * 4. Additional check regarding server are performed
	 * 5. If verbal additional info is printed
	 *
	 * @return GSoapContexAdapter instance, or null if all activities
	 * 				requested using program options have been done.
	 */
	virtual GSoapContextAdapter* validate();

	/**
	 * Creates job elements.
	 *
	 * The job elements are created based on used command line options
	 * or based on the given bulk-job file. The created elements are
	 * stored in an internal vector.
	 *
	 * @return true is the internal vector containing job elements is not empty
	 */
	bool createJobElements();

	/**
	 * Performs standard checks.
	 *
	 *  	- decides on delegation (checks whether it is supported)
	 *  	- collects user password if needed
	 *  	- checks if the job is specified twice (using command line and bulk-job file)
	 *  	- checks if used options are supported (e.g. checksum)
	 */
	bool performChecks();

	/**
	 * Gives the instruction how to use the command line tool.
	 *
	 * @return a string with instruction on how to use the tool
	 */
	string getUsageString(string tool);

	/**
	 * Gets the password (if set using -p option or by performChecks()
	 *
	 * @return password string
	 *
	 * @see SubmitTransferCli::performChecks()
	 */
	string getPassword();

	/**
	 * Gets the source file name (string) for the job.
	 *
	 * @return source string if it was given as a CLI option, or an empty string if not
	 */
	string getSource();

	/**
	 * Gets the destination file name (string) for the job.
	 *
	 * @return destination string if it was given as a CLI option, or an empty string if not
	 */
	string getDestination();

	/**
	 * Gets the 'tns3__TransferParams' object.
	 *
	 * The parameters are set accordingly to the options used with the command line tool.
	 * The object is created using gSOAP memory-allocation utility, it will be garbage
	 * collected! If there is a need to delete it manually gSOAP dedicated functions should
	 * be used (in particular 'soap_unlink'!)
	 *
	 * @return tns3__TransferParams object containing name-value pairs
	 */
	tns3__TransferParams* getParams();

	/**
	 * Gets a vector containing 'tns3__TransferJobElement' objects.
	 *
	 * The returned vector is created based on the internal vector created using 'createJobElements()'.
	 * Each of the vector elements is created using gsoap memory-allocation utility, and will
	 * be garbage collected. If there is a need to delete it manually gsoap dedicated functions should
	 * be used (in particular 'soap_unlink'!)
	 *
	 * @return vector of 'tns3__TransferJobElement' objects
	 *
	 * @see SubmitTransferCli::createJobElements()
	 */
	vector<tns3__TransferJobElement * > getJobElements();

	/**
	 * Gets a vector containing 'tns3__TransferJobElement2' objects.
	 *
	 * The returned vector is created based on the internal vector created using 'createJobElements()'.
	 * Each of the vector elements is created using gSOAP memory-allocation utility, and will
	 * be garbage collected. If there is a need to delete it manually gsoap dedicated functions should
	 * be used (in particular 'soap_unlink'!)
	 *
	 * @return vector of 'tns3__TransferJobElement2' objects
	 *
	 * @see SubmitTransferCli::createJobElements()
	 */
	vector<tns3__TransferJobElement2 * > getJobElements2();

	/**
	 * Gets the value of delegation flag.
	 *
	 * @return true if the proxy certificate should be delegated
	 */
	bool useDelegation() {
		return delegate;
	}

	/**
	 * Gets the value of checksum flag.
	 *
	 * @return true if checksum is used
	 */
	bool useCheckSum() {
		return checksum;
	}

	/**
	 * Check if the blocking mode is on.
	 *
	 * @return true if the -b option has been used
	 */
	bool isBlocking();

private:

	/**
	 * Ask user for password.
	 *
	 * @return user password
	 */
	string askForPassword();

	/**
	 * the name of the file containing bulk-job description
	 */
	string bulk_file;

	/**
	 * user password
	 */
	string password;

	/**
	 * internal vector containing job elements
	 *
	 * @see SubmitTransferCli::createJobElements(), SubmitTransferCli::getJobElements(soap*) , SubmitTransferCli::getJobElements2(soap*)
	 */
	vector<JobElement> tasks;

	/**
	 * checksum flag, determines whether checksum is used
	 */
	bool checksum;

	/**
	 * delegate flag, determines whether the proxy certificate should be used
	 */
	bool delegate;

};

}
}

#endif /* SUBMITTRANSFERCLI_H_ */

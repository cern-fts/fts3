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
 *  Created on: May 30, 2012
 *      Author: Michał Simon
 */

#ifndef GSOAPCONTEXADAPTER_H_
#define GSOAPCONTEXADAPTER_H_

#include "ws-ifce/gsoap/gsoap_stubs.h"

#include <string>

using namespace std;

namespace fts3 { namespace cli {


/**
 * The adapter class for the GSoap context
 *
 * This one is used instead of GSoap generated proxy classes because of GSoap bug ID: 3511337
 * (see example http://sourceforge.net/tracker/?func=detail&atid=468021&aid=3511337&group_id=52781)
 *
 * Provides all the functionalities of transfer and configuration web service
 */
class GSoapContextAdapter {

public:

	/**
	 * Constructor
	 *
	 * Creates and initializes GSoap context
	 */
	GSoapContextAdapter(string endpoint);

	/**
	 * Destructor
	 *
	 * Deallocates GSoap context
	 */
	virtual ~GSoapContextAdapter();

	/**
	 * Type cast operator.
	 *
	 * @return pointer do gsoap context (soap*)
	 */
	operator soap*() {
		return ctx;
	}


	/**
	 * Initializes CGSI plugin for the soap object, respectively to the protocol that is used (https, httpg,..).
	 * 		Moreover, sets the namespaces.
	 * Initializes the object with service version, metadata, schema version and interface version.
	 *
	 * @return true if all information were received
	 * @see SrvManager::initSoap(soap*, string)
	 */
	void init();

	/**
	 * Prints general information about the FTS3 service.
	 * Should be used if the -v option has been used.
	 */
	void printInfo();

	/**
	 * Handles soap fault. Calls soap_stream_fault, then throws a string exception with given message
	 *
	 * @param msg exception message
	 */
	void handleSoapFault(string msg);


	/**
	 * Remote call that will be transferSubmit3
	 *
	 * @param job the job to be executed
	 * @param resp server response (job id)
	 */
	void transferSubmit3 (tns3__TransferJob2* job, impltns__transferSubmit3Response& resp);

	/**
	 * Remote call to transferSubmit2
	 *
	 * @param job the job that will be executed
	 * @param resp server response (job id)
	 */
	void transferSubmit2 (tns3__TransferJob* job, impltns__transferSubmit2Response& resp);

	/**
	 * Remote call to transferSubmit
	 *
	 * @param job the job that will be executed
	 * @param resp server response (job id)
	 */
	void transferSubmit (tns3__TransferJob* job, impltns__transferSubmitResponse& resp);

	/**
	 * Remote call to getTransferJobStatus
	 *
	 * @param jobId the job id
	 * @param resp server response (status)
	 */
	void getTransferJobStatus (string jobId, impltns__getTransferJobStatusResponse& resp);

	/**
	 * Remote call to getRoles
	 *
	 * @param resp server response (roles)
	 */
	void getRoles (impltns__getRolesResponse& resp);

	/**
	 * Remote call to getRolesOf
	 *
	 * @param resp server response (roles)
	 */
	void getRolesOf (string dn, impltns__getRolesOfResponse& resp);

	/**
	 * Remote call to cancel
	 *
	 * @param rqst list of job ids
	 * @param resp server response
	 */
	void cancel(impltns__ArrayOf_USCOREsoapenc_USCOREstring* rqst, impltns__cancelResponse& resp);

	/**
	 * Remote call to listRequests2
	 *
	 * @param dn user dn
	 * @param vo vo name
	 * @param array statuses of interest
	 * @param resp server response
	 */
	void listRequests2 (impltns__ArrayOf_USCOREsoapenc_USCOREstring* array, string dn, string vo, impltns__listRequests2Response& resp);

	/**
	 * Remote call to listRequests
	 *
	 * @param array statuses of interest
	 * @param resp server response
	 */
	void listRequests (impltns__ArrayOf_USCOREsoapenc_USCOREstring* array, impltns__listRequestsResponse& resp);

	/**
	 * Remote call to listVOManagers
	 *
	 * @param vo vo name
	 * @param resp server response
	 */
	void listVoManagers (string vo, impltns__listVOManagersResponse& resp);

	/**
	 * Remote call to getTransferJobSummary2
	 *
	 * @param jobId id of the job
	 * @param resp server response
	 */
	void getTransferJobSummary2 (string jobId, impltns__getTransferJobSummary2Response& resp);

	/**
	 * Remote call to getTransferJobSummary
	 *
	 * @param jobId id of the job
	 * @param resp server response
	 */
	void getTransferJobSummary (string jobId, impltns__getTransferJobSummaryResponse& resp);

	/**
	 * Remote call to getFileStatus
	 *
	 * @param jobId id of the job
	 * @param resp server response
	 */
	void getFileStatus (string jobId, impltns__getFileStatusResponse& resp);

	/**
	 * Remote call to setConfiguration
	 *
	 * @param config th configuration to be set
	 * @param resp server response
	 */
	void setConfiguration (config__Configuration *config, implcfg__setConfigurationResponse& resp);

	/**
	 * Remote call to getConfiguration
	 *
	 * @param vo - vo name that will be used to filter the response
	 * @param name - SE or SE group name that will be used to filter the response
	 * @param resp - server response
	 */
	void getConfiguration (string vo, string name, implcfg__getConfigurationResponse& resp);

	/**
	 * Remote call to delConfiguration
	 *
	 * @param cfg - the configuration that will be deleted
	 * @param resp - server response
	 */
	void delConfiguration(config__Configuration *config, implcfg__delConfigurationResponse &resp);

	/**
	 * Splits the given string, and sets:
	 * 		- major number
	 * 		- minor number
	 * 		- patch number
	 *
	 * @param interface - interface version of FTS3 service
	 */
	void setInterfaceVersion(string interface);

	/**
	 * TODO
	 */
	void debugSet(string source, string destination, bool debug);

	/**
	 * TODO
	 */
	void doDrain(bool drain);

	///@{
	/**
	 * A group of methods that check if the FTS3 service supports various functionalities.
	 * TODO legacy verify if really needed
	 */
	int isChecksumSupported();
	int isDelegationSupported();
	int isRolesOfSupported();
	int isSetTCPBufferSupported();
	int isUserVoRestrictListingSupported();
	int isVersion330StatesSupported();
	///@}


	///@{
	/**
	 * A group of methods that check the version of the FTS3 service
	 * TODO legacy verify if really needed
	 */
	int isItVersion330();
	int isItVersion340();
	int isItVersion350();
	int isItVersion370();
	///@}


	///@{
	/**
	 * A group of methods returning details about interface version of the FTS3 service
	 */
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
	///@}

private:

	///
	string endpoint;

	///
	soap* ctx;

	///@{
	/**
	 * Interface Version components
	 */
	long major;
	long minor;
	long patch;
	///@}

	///@{
	/**
	 * general informations about the FTS3 service
	 */
	string interface;
	string version;
	string schema;
	string metadata;
	///@}

};

}
}

#endif /* GSOAPCONTEXADAPTER_H_ */

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
 * SrvManager.h
 *
 *  Created on: Feb 7, 2012
 *      Author: simonm
 */

#ifndef SRVMANAGER_H_
#define SRVMANAGER_H_

#include <string>
#include "gsoap_proxy.h"

using namespace std;


namespace fts3 { namespace cli {

/**
 * SrvManager object stores the information about the FTS3 service that is being used.
 * 		Moreover, it is responsible for security issues (CGSI, Proxy Certificate delegation)
 *
 * 	SrvManager is implemented using singleton design pattern, because only one FTS3 service
 * 	is used by a fts3-transfer utility at run-time, and the data describing this service
 * 	are required at various points by several objects.
 */
class SrvManager {
public:

	/**
	 * Gets the instance of SrvManager singleton.
	 *
	 * @return SrvManager instance
	 */
	static SrvManager* getInstance();

	/**
	 * Destructor.
	 */
	virtual ~SrvManager();

	/**
	 * Initializes CGSI plugin for the soap object, respectively to the protocol that is used (https, httpg,..).
	 * 		Moreover, sets the namespaces.
	 *
	 * @param soap - soap object corresponding to the FTS3 service
	 * @param endpoint - URL of the FTS3 service
	 *
	 * @return true if the operation was successful
	 */
	bool initSoap(soap* soap, string endpoint);

	/**
	 * Initializes the object with service version, metadata, schema version and interface version.
	 *
	 * Usually, the soap object has be first initialized using 'initSoap(soap*, string)'
	 *
	 * @param service - proxy to the FTS3 service
	 *
	 * @return true if all information were received
	 * @see SrvManager::initSoap(soap*, string)
	 */
	bool init(FileTransferSoapBindingProxy& service);

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
	 * Check if user's certificate is valid.
	 *
	 * @return remaining time for user's certificate in seconds.
	 */
	long isCertValid ();

	/**
	 * Delegates the proxy certificate.
	 *
	 * Not supported yet!
	 *
	 * @param endpoint - URL of the FTS3 service
	 *
	 */
	void delegateProxyCert(string endpoint);

	/**
	 * Prints the soap fault message in case there was an error
	 *
	 * #param service - proxy to the FTS3 service
	 *
	 */
	void printSoapErr (FileTransferSoapBindingProxy& service);

	///@{
	/**
	 * A group of methods that check if the FTS3 service supports various functionalities.
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

protected:
	/**
	 * Default constructor (private).
	 */
	SrvManager();

	///@{
	/**
	 * Interface Version components
	 */
	long major;
	long minor;
	long patch;
	///@}

private:
	/**
	 * Copy constructor (private).
	 */
	SrvManager (SrvManager const&);

	/**
	 * Copy assignment operator (private).
	 */
	SrvManager& operator=(SrvManager const&);

	/**
	 * SrvManager single instance
	 */
	static SrvManager* manager;

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

#endif /* SRVMANAGER_H_ */

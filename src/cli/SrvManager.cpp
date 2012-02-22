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
 * CliManager.cpp
 *
 *  Created on: Feb 7, 2012
 *      Author: simonm
 */

#include "SrvManager.h"

#include <openssl/x509_vfy.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/asn1.h>

#include <termios.h>
#include <cgsi_plugin.h>
#include <gridsite.h>

#include <time.h>
#include <iostream>

#include "GsoapStubs.h"

#include <boost/tokenizer.hpp>
#include <boost/lexical_cast.hpp>

#include "fts.nsmap"

using namespace boost;
using namespace fts::cli;

// define the status name - status id pairs
SrvManager::TransferState SrvManager::statuses[] = {
		{ "Submitted",      			FTS3_TRANSFER_SUBMITTED },
		{ "Pending",        			FTS3_TRANSFER_PENDING },
		{ "Active",     				FTS3_TRANSFER_ACTIVE },
		{ "Canceling",      			FTS3_TRANSFER_CANCELING },
		{ "Done",       				FTS3_TRANSFER_DONE },
		{ "Finished",       			FTS3_TRANSFER_FINISHED },
		{ "FinishedDirty",  			FTS3_TRANSFER_FINISHED_DIRTY },
		{ "Canceled",       			FTS3_TRANSFER_CANCELED },
		{ "Waiting",        			FTS3_TRANSFER_WAITING },
		{ "Hold",       				FTS3_TRANSFER_HOLD },
		{ "Failed",     				FTS3_TRANSFER_TRANSFERFAILED },
		{ "CatalogFailed",  			FTS3_TRANSFER_CATALOGFAILED },
		{ "Ready",   					FTS3_TRANSFER_READY },
		{ "DoneWithErrors", 			FTS3_TRANSFER_DONEWITHERRORS },
		{ "Finishing", 					FTS3_TRANSFER_FINISHING },
		{ "Failed", 					FTS3_TRANSFER_FAILED },
		{ "AwaitingPrestage" , 			FTS3_TRANSFER_AWAITING_PRESTAGE },
		{ "Prestaging", 				FTS3_TRANSFER_PRESTAGING },
		{ "WaitingPrestage", 			FTS3_TRANSFER_WAITING_PRESTAGE },
		{ "WaitingCatalogResolution", 	FTS3_TRANSFER_WAITING_CATALOG_RESOLUTION },
		{ "WaitingCatalogRegistration", FTS3_TRANSFER_WAITING_CATALOG_REGISTRATION}
	};

// initialize the single SrvManager instance
SrvManager* SrvManager::manager = 0;

SrvManager::SrvManager() {

}

SrvManager::SrvManager(SrvManager const&) {

}

SrvManager& SrvManager::operator=(SrvManager const&) {
	return *this;
}

SrvManager::~SrvManager() {
	delete manager;
}

SrvManager* SrvManager::getInstance() {

	// lazy loading
	if (!manager) {
		manager = new SrvManager();
	}
	return SrvManager::manager;
}

bool SrvManager::isTransferReady(string status) {

	// find the transfer status in statuses table
	for (int i = 0; i < statusesSize; i++) {
		if (status.compare(statuses[i].name) == 0) {
			// check if its ready
			return statuses[i].id <= 0;
		}
	}

	return FTS3_TRANSFER_UNKNOWN <= 0;
}

// TODO think if its the right place for this method, maybe it should be moved to SubmitTransferCli!
string SrvManager::getPassword() {

    termios stdt;
    // get standard command line settings
    tcgetattr(STDIN_FILENO, &stdt);
    termios newt = stdt;
    // turn off echo while typing
    newt.c_lflag &= ~ECHO;
    if (tcsetattr(STDIN_FILENO, TCSANOW, &newt)) {
    	cout << "submit: could not set terminal attributes" << endl;
    	tcsetattr(STDIN_FILENO, TCSANOW, &stdt);
    	return "";
    }

    string pass1, pass2;

    cout << "Enter MyProxy password: ";
    cin >> pass1;
    cout << endl << "Enter MyProxy password again: ";
    cin >> pass2;
    cout << endl;

    // set the standard command line settings back
    tcsetattr(STDIN_FILENO, TCSANOW, &stdt);

    // compare passwords
    if (pass1.compare(pass2)) {
    	cout << "Entered MyProxy passwords do not match." << endl;
    	return "";
    }

    return pass1;
}

bool SrvManager::initSoap(soap* soap, string endpoint) {

	// initialize CGSI depending on the protocol (https, httpg)
    int  ret = 0;
    if (endpoint.find("https") == 0) {
    	ret = soap_cgsi_init(soap,  CGSI_OPT_DISABLE_NAME_CHECK | CGSI_OPT_SSL_COMPATIBLE);
    } else if (endpoint.find("httpg") == 0) {
    	ret = soap_cgsi_init(soap, CGSI_OPT_DISABLE_NAME_CHECK);
    }

    if (ret) {
    	cout << "Failed to initialize the GSI plugin." << endl;
    	return false;
    }

    // set the namespaces
	if (soap_set_namespaces(soap, fts_namespaces)) {
		cout << "Failed to set SOAP namespaces." << endl;
		return false;
	}

	return true;
}

void SrvManager::delegateProxyCert(string endpoint) {

	// is there a reson for glite_discover_endpoint ???

	/*string replacement = "/services/gridsite-delegation";
	int pos = service.find("/services/FileTransfer");
	service.replace(pos, replacement.size(), replacement);
*/
	time_t time_left = isCertValid();

	// TODO
	// delegation-simple-api not compatible with gsoap++ due to C version of cgsi_plugin

}

long SrvManager::isCertValid () {

	// find user proxy certificate
    char * user_proxy = GRSTx509FindProxyFileName();
	FILE *fp = fopen(user_proxy , "r");
	// read the certificate
    X509 *cert = PEM_read_X509(fp, 0, 0, 0);
    fclose(fp);
    char* c_str = (char*) ASN1_STRING_data(X509_get_notAfter(cert));
    // calculate the time remaing for the proxy certificate
    long time = GRSTasn1TimeToTimeT(c_str, 0) - ::time(0);

    cout << "Remaining time for local proxy is " << time / 3600 << " hours and " << time % 3600 / 60 << " minutes." << endl;

    return time;
}

bool SrvManager::init(FileTransferSoapBindingProxy& service) {

	// request the information about the FTS3 service

	int err;

	fts__getInterfaceVersionResponse ivresp;
	err = service.getInterfaceVersion(ivresp);
	if (!err) {
		interface = ivresp.getInterfaceVersionReturn;
		setInterfaceVersion(interface);
	}

	fts__getVersionResponse vresp;
	err = service.getVersion(vresp);
	if (!err) {
		version = vresp.getVersionReturn;
	}

	fts__getSchemaVersionResponse sresp;
	err = service.getSchemaVersion(sresp);
	if (!err) {
		schema = sresp.getSchemaVersionReturn;
	}

	fts__getServiceMetadataResponse mresp;
	err = service.getServiceMetadata("feature.string", mresp);
	if (!err) {
		metadata = mresp._getServiceMetadataReturn;
	}

	return err;
}

void SrvManager::setInterfaceVersion(string interface) {

	if (interface.empty()) return;


	// set the seperator that will be used for tokenizing
	char_separator<char> sep(".");
	tokenizer< char_separator<char> > tokens(interface, sep);
	tokenizer< char_separator<char> >::iterator it = tokens.begin();

	if (it == tokens.end()) return;

	string s = *it++;
	major = lexical_cast<long>(s);

	if (it == tokens.end()) return;

	s = *it++;
	minor = lexical_cast<long>(s);

	if (it == tokens.end()) return;

	s = *it;
	patch = lexical_cast<long>(s);
}

int SrvManager::isChecksumSupported() {
    return isItVersion370();
}

int SrvManager::isDelegationSupported() {
    return isItVersion330();
}

int SrvManager::isRolesOfSupported() {
    return isItVersion330();
}

int SrvManager::isSetTCPBufferSupported() {
    return isItVersion330();
}

int SrvManager::isUserVoRestrictListingSupported() {
    return isItVersion330();
}

int SrvManager::isVersion330StatesSupported(){
    return isItVersion330();
}

int SrvManager::isItVersion330() {
    return major >= 3 && minor >= 3 && patch >= 0;
}

int SrvManager::isItVersion340() {
    return major >= 3 && minor >= 4 && patch >= 0;
}

int SrvManager::isItVersion350() {
    return major >= 3 && minor >= 5 && patch >= 0;
}

int SrvManager::isItVersion370() {
	return major >= 3 && minor >= 7 && patch >= 0;
}


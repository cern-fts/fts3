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
 * SrvManager.cpp
 *
 *  Created on: Feb 7, 2012
 *      Author: Michal Simon
 */

#include "SrvManager.h"
/*
#include <openssl/x509_vfy.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/asn1.h>
*/
#include <cgsi_plugin.h>
#include <iostream>
//#include <gridsite.h>

#include <time.h>
#include <iostream>

#include <boost/tokenizer.hpp>
#include <boost/lexical_cast.hpp>

#include "ws-ifce/gsoap/fts3.nsmap"

using namespace std;
using namespace boost;
using namespace fts3::cli;

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
	if (manager) {
		delete manager;
	}
}

SrvManager* SrvManager::getInstance() {

	// lazy loading
	if (!manager) {
		manager = new SrvManager();
	}
	return SrvManager::manager;

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
	if (soap_set_namespaces(soap, fts3_namespaces)) {
		cout << "Failed to set SOAP namespaces." << endl;
		return false;
	}

	return true;
}

void SrvManager::delegateProxyCert(string) {

	// is there a reson for glite_discover_endpoint ???

	/*string replacement = "/services/gridsite-delegation";
	int pos = service.find("/services/FileTransfer");
	service.replace(pos, replacement.size(), replacement);

	time_t time_left = isCertValid();
	*/
	// TODO
	// delegation-simple-api not compatible with gsoap++ due to C version of cgsi_plugin

}

long SrvManager::isCertValid () {
/*
	// find user proxy certificate
    char* user_proxy = GRSTx509FindProxyFileName();
	FILE *fp = fopen(user_proxy , "r");
	// read the certificate
    X509 *cert = PEM_read_X509(fp, 0, 0, 0);
    fclose(fp);
    char* c_str = (char*) ASN1_STRING_data(X509_get_notAfter(cert));
    // calculate the time remaing for the proxy certificate
    long time = GRSTasn1TimeToTimeT(c_str, 0) - ::time(0);

    cout << "Remaining time for local proxy is " << time / 3600 << " hours and " << time % 3600 / 60 << " minutes." << endl;
*/
	long time = 0;
    return time;
}

void SrvManager::printSoapErr(FileTransferSoapBindingProxy& service) {

	if (service.fault && service.fault->faultstring) {
		cout << service.fault->faultstring << endl;
	}
}

bool SrvManager::init(FileTransferSoapBindingProxy& service) {

	// request the information about the FTS3 service

	int err;

	impltns__getInterfaceVersionResponse ivresp;
	err = service.getInterfaceVersion(ivresp);
	if (!err) {
		interface = ivresp.getInterfaceVersionReturn;
		setInterfaceVersion(interface);
	} else {
		cout << "Failed to determine the interface version of the service: getInterfaceVersion. ";
		printSoapErr(service);
		return false;
	}

	impltns__getVersionResponse vresp;
	err = service.getVersion(vresp);
	if (!err) {
		version = vresp.getVersionReturn;
	} else {
		cout << "Failed to determine the version of the service: getVersion. ";
		printSoapErr(service);
		return false;
	}

	impltns__getSchemaVersionResponse sresp;
	err = service.getSchemaVersion(sresp);
	if (!err) {
		schema = sresp.getSchemaVersionReturn;
	} else {
		cout << "Failed to determine the schema version of the service: getSchemaVersion. ";
		printSoapErr(service);
		return false;
	}

	impltns__getServiceMetadataResponse mresp;
	err = service.getServiceMetadata("feature.string", mresp);
	if (!err) {
		metadata = mresp._getServiceMetadataReturn;
	} else {
		cout << "Failed to determine the service metadata of the service: getServiceMetadataReturn. ";
		printSoapErr(service);
		return false;
	}

	return true;
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

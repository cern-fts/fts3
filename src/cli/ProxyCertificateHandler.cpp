/*
 * ProxyCertificateHandler.cpp
 *
 *  Created on: Jun 4, 2012
 *      Author: simonm
 */

#include "ProxyCertificateHandler.h"

#include <openssl/x509_vfy.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/asn1.h>

#include <gridsite.h>

#include <iostream>

ProxyCertificateHandler::ProxyCertificateHandler(string endpoint, string delegationId, int userDefinedExpTime):
		user_requested_delegation_exp_time(userDefinedExpTime),
		delegationId(delegationId),
		endpoint(endpoint) {

}

ProxyCertificateHandler::~ProxyCertificateHandler() {

}

void ProxyCertificateHandler::delegateProxyCert(string) {

	// is there a reason for glite_discover_endpoint ???

	/*string replacement = "/services/gridsite-delegation";
	int pos = service.find("/services/FileTransfer");
	service.replace(pos, replacement.size(), replacement);

	time_t time_left = isCertValid();
	*/
	// TODO
	// delegation-simple-api not compatible with gsoap++ due to C version of cgsi_plugin

}

long ProxyCertificateHandler::isCertValid(string filename) {

	// find user proxy certificate
	FILE* fp;
	if (!filename.empty()) {
		fp = fopen(filename.c_str(), "r");
	} else {
		char* user_proxy = GRSTx509FindProxyFileName();
		fp = fopen(user_proxy , "r");
	}

	// if file could not be opened return 0
	if (!fp) return 0;

	// read the certificate
    X509 *cert = PEM_read_X509(fp, 0, 0, 0);
    fclose(fp);

    // if the cert could not be read return 0
    if (!cert) return 0;

    char* c_str = (char*) ASN1_STRING_data(X509_get_notAfter(cert));
    // calculate the time remaing for the proxy certificate
    long time = GRSTasn1TimeToTimeT(c_str, 0) - ::time(0);

    return time;
}

bool ProxyCertificateHandler::delegate() {

    glite_delegation_ctx *dctx;
    int force_delegation;
    int need_delegation;
    int ret;
    time_t exp_time, current_time, request_proxy_delegation_time, time_left_on_server, local_proxy_time_left;

    dctx = glite_delegation_new(endpoint.c_str());
    if (dctx == NULL) {
        cout << "delegation: could not initialize a delegation context" << endl;
        return false;
    }

    // get local proxy run time
    local_proxy_time_left = isCertValid(string());
    cout << "Remaining time for local proxy is "
    	<< (long int)((local_proxy_time_left) / 3600)
    	<< "hours and "
    	<< (long int)((local_proxy_time_left) % 3600 / 60)
    	<< " minutes." << endl;

    // the default is to delegate
    need_delegation = 1;
    // ..but not renew
    force_delegation = 0;
    // todo: why is this called "force_delegation" ?!?

    // checking if there is already a delegated credenial
    ret = glite_delegation_info(dctx, delegationId.c_str(), &exp_time);

    if (0 == ret) {   // there is already a delegated proxy on the server
        current_time = time(NULL);
        time_left_on_server = exp_time - current_time;

        if (time_left_on_server > redelegation_time_limit) {
            // don;t bother redelegating
            cout << "Not bothering to do delegation, since the server already has a delegated credential for this user lasting longer than 4 hours." << endl;
            cout << "Remaining time on server for this credential is "
                 << (long int)((time_left_on_server) / 3600)
                 << " hours and "
                 << (long int)((time_left_on_server) % 3600 / 60)
                 << " minutes." << endl;
            need_delegation = 0;
            force_delegation = 0;
        } else {
            // think about redelegating
            if (local_proxy_time_left > time_left_on_server) {
                // we improve the situation (the new proxy will last longer)
                cout << "Will redo delegation since the credential on the server has left that 4 hours validity left." << endl;
                cout << "Remaining time for this credential is "
                     << (long int)((time_left_on_server) / 3600)
                     << " hours and "
                     << (long int)((time_left_on_server) % 3600 / 60)
                     << " minutes." << endl;
                need_delegation = 1;
                force_delegation = 1; // renew rather than put
            } else {
                // we cannot improve the proxy on the server
                cout << "Delegated proxy on server has less than 4 hours left." << endl;
                cout << "But the local proxy has less time left than the one on the server, so cannot be used to refresh it!" << endl;
                cout << "Remaining time on server for this credential is "
                     << (long int)((time_left_on_server) / 3600)
                     << " hours and "
                     << (long int)((time_left_on_server) % 3600 / 60)
                     << " minutes." << endl;
                cout << "Remaining time for local proxy is "
                     << (long int)((local_proxy_time_left) / 3600)
                     << " hours and "
                     << (long int)((local_proxy_time_left) % 3600 / 60)
                     << " minutes." << endl;
                need_delegation=0;
            }
        }
    } else {
        // no proxy on server: do standard delegation
    	cout << "No proxy found on server. Requesting standard delegation" << endl;
        need_delegation = 1;
        force_delegation = 0;
    }

    if (need_delegation) {
        if (user_requested_delegation_exp_time == 0) {
            request_proxy_delegation_time = local_proxy_time_left - 60; // 60 seconds off current proxy
            if (request_proxy_delegation_time > maximum_time_for_delegation_request ) {
                request_proxy_delegation_time = maximum_time_for_delegation_request;
            }
            if (request_proxy_delegation_time <= 0) {
                // using a proxy with less than 1 minute to go
                cout << "Your local proxy has less than 1 minute to run, Please renew it before submitting a job." << endl;
                return false;
            }
        } else {
            request_proxy_delegation_time = user_requested_delegation_exp_time;
        }
        cout << "Requesting delegated proxy for "
             << (long int)((request_proxy_delegation_time) / 3600)
             << " hours and "
             << (long int)((request_proxy_delegation_time) % 3600 / 60)
             << " minutes." << endl;
        ret = glite_delegation_delegate(dctx, delegationId.c_str(),
                                        (request_proxy_delegation_time/60),
                                        force_delegation);
        if (-1 == ret) {
            cout << "delegation: " << glite_delegation_get_error(dctx) << endl;
            return false;
        }
        cout << "Credential is successfully delegated to the service." << endl;
    }

    glite_delegation_free(dctx);

    return true;
}

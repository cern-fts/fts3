/*
 * CGsiAdapter.cpp
 *
 *  Created on: Nov 1, 2012
 *      Author: simonm
 */

#include "CGsiAdapter.h"

#include <cgsi_plugin.h>

#include <openssl/pem.h>
#include <openssl/x509.h>

#include <boost/regex.hpp>

#include "common/error.h"

namespace fts3 {
namespace ws {

using namespace fts3::common;
using namespace boost;


const string CGsiAdapter::hostDn = CGsiAdapter::initHostDn();


CGsiAdapter::CGsiAdapter(soap* ctx) : ctx(ctx) {

	// get client VO
	char* tmp = get_client_voname(ctx);
	if(tmp) vo = tmp;

	// get client DN
	const int len = 200;
	char buff[len];
	if (get_client_dn(ctx, buff, len)) throw Err_Custom("'get_client_dn' failed!");
	dn = buff;

	// retrieve VOMS attributes (fqnas)
	int nbfqans = 0;
	char **arr = get_client_roles(ctx, &nbfqans);

	if (nbfqans == 0) {
		// if the host certificate was used to submit the request we will not find any fqans
		if (hostDn.empty() || dn != hostDn)
			throw Err_Custom("Failed to extract VOMS attributes from Proxy Certificate (probably the CRL has expired)!");
	}

	for (int i = 0; i < nbfqans; i++) {
		attrs.push_back(arr[i]);
	}
}

CGsiAdapter::~CGsiAdapter() {
}

string CGsiAdapter::getClientVo() {
	return vo;
}

string CGsiAdapter::getClientDn() {
	return dn;
}

vector<string> CGsiAdapter::getClientAttributes() {
	return attrs;
}

vector<string> CGsiAdapter::getClientRoles() {

	static const regex re ("/.*/Role=(\\w+)/.*");
	static const int ROLE_VALUE = 1;

	vector<string> ret;
	vector<string>::iterator it;

	for (it = attrs.begin(); it != attrs.end(); it++) {

		smatch what;
		regex_match(*it, what, re, match_extra);
		ret.push_back(what[ROLE_VALUE]);
	}

	return ret;
}

string CGsiAdapter::initHostDn() {

	// TODO check if other location is not used for hostcert.pem
	// default path to host certificate
	const string hostCert = "/etc/grid-security/hostcert.pem";
	string dn;
    // check the server host certificate
	FILE *fp = fopen(hostCert.c_str(), "r");
	X509 *cert = PEM_read_X509(fp, 0, 0, 0);

	if (!cert) return string();

	dn = cert->name;
	X509_free(cert);
	fclose(fp);

	return dn;
}

} /* namespace ws */
} /* namespace fts3 */

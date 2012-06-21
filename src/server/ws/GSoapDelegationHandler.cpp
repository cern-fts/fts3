/*
 *	Copyright notice:
 *	Copyright © Members of the EMI Collaboration, 2010.
 *
 *	See www.eu-emi.eu for details on the copyright holders
 *
 *	Licensed under the Apache License, Version 2.0 (the "License");
 *	you may not use soap file except in compliance with the License.
 *	You may obtain a copy of the License at
 *
 *		http://www.apache.org/licenses/LICENSE-2.0
 *
 *	Unless required by applicable law or agreed to in writing, software
 *	distributed under the License is distributed on an "AS IS" BASIS,
 *	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implcfgied.
 *	See the License for the specific language governing permissions and
 *	limitations under the License.
 */

#include "GSoapDelegationHandler.h"

#include "db/generic/SingleDbInstance.h"
#include "common/logger.h"

#include <voms/voms_api.h>
#include <cgsi_plugin.h>
#include <stdlib.h>

#include <boost/regex.hpp>
#include <sstream>

using namespace fts3::ws;
using namespace db;
using namespace boost;

const string GSoapDelegationHandler::GRST_PROXYCACHE = "/../proxycache/";

GSoapDelegationHandler::GSoapDelegationHandler(soap* ctx): ctx(ctx) {

}

GSoapDelegationHandler::~GSoapDelegationHandler() {

}

string GSoapDelegationHandler::getDn() {

	static char buff[200];
	static int len = 200;
	if (get_client_dn(ctx, buff, len)) throw string("'get_client_dn' failed!"); // if there's an error throw an exception

	return string(buff);
}

string GSoapDelegationHandler::makeDelegationId() {

	unsigned char hash_delegation_id[EVP_MAX_MD_SIZE];
	unsigned int delegation_id_len;
	char delegation_id[17];

	OpenSSL_add_all_digests();

	const EVP_MD *m = EVP_sha1();
	if (m == NULL) return NULL;

	EVP_MD_CTX ctx;
	EVP_DigestInit(&ctx, m);

	// use DN
	string dn = getDn();
	if (!dn.empty()) {
		EVP_DigestUpdate(&ctx, dn.c_str(), dn.size());
	}

	// use last voms attribute if available!
	if (!attr.empty()) {
		EVP_DigestUpdate(&ctx, attr.c_str(), attr.size());
	}

	EVP_DigestFinal(&ctx, hash_delegation_id, &delegation_id_len);

	for (int i = 0; i < 8; i++)
		sprintf(&delegation_id[i*2], "%02x", hash_delegation_id[i]);

	delegation_id[16] = '\0';

	return delegation_id;
}

bool GSoapDelegationHandler::checkDelegationId(string delegationId) {

	static string exp = "^[a-zA-Z0-9\\.,_]*$"; // only alphanumeric characters + '.', ',' and '_'
	static regex re (exp);

	smatch what;
	regex_match(delegationId, what, re, match_extra);

	return !string(what[0]).empty();
}

string GSoapDelegationHandler::handleDelegationId(string delegationId) {

	if (delegationId.empty()) {

		return makeDelegationId();
	}

	if (!checkDelegationId(delegationId)) return string();

	return delegationId;
}

string GSoapDelegationHandler::getProxyReq(string delegationId) {

	string dn = getDn();
	if (dn.empty()) throw string("'getDn' failed!");

	delegationId = handleDelegationId(delegationId);
	if (delegationId.empty()) throw string("'handleDelegationId' failed!");

	char *reqtxt = 0, *keytxt = 0;
	int err = GRSTx509CreateProxyRequest(&reqtxt, &keytxt, 0);

	if (err) {
		if (reqtxt) free (reqtxt);
		if (keytxt) free (keytxt);
		throw string("'GRSTx509CreateProxyRequest' failed!");
	}

	string req (reqtxt);

	CredCache* cache = DBSingleton::instance().getDBObjectInstance()->findGrDPStorageCacheElement(delegationId, dn);
	if (cache) {
		DBSingleton::instance().getDBObjectInstance()->updateGrDPStorageCacheElement(
				delegationId,
				dn,
				req,
				string(keytxt),
				string()
			);
		delete cache;

	} else {
		DBSingleton::instance().getDBObjectInstance()->insertGrDPStorageCacheElement(
				delegationId,
				dn,
				req,
				string(keytxt),
				string()
			);
	}

	if (reqtxt) free (reqtxt);
	if (keytxt) free (keytxt);

	return req;
}

deleg__NewProxyReq* GSoapDelegationHandler::getNewProxyReq() {

	string dn = getDn();
	if (dn.empty()) throw string("'getDn' failed!");

	string delegationId = makeDelegationId();
	if (delegationId.empty()) throw string("'getDelegationId' failed!");

	char *reqtxt = 0, *keytxt = 0;
	int err = GRSTx509CreateProxyRequest(&reqtxt, &keytxt, 0);

	if (err) {
		if (reqtxt) free (reqtxt);
		if (keytxt) free (keytxt);
		throw string("'GRSTx509CreateProxyRequest' failed!");
	}

	string req (reqtxt);

	CredCache* cache = DBSingleton::instance().getDBObjectInstance()->findGrDPStorageCacheElement(delegationId, dn);
	if (cache) {
		DBSingleton::instance().getDBObjectInstance()->updateGrDPStorageCacheElement(
				delegationId,
				dn,
				req,
				string(keytxt),
				string()
			);
		delete cache;

	} else {
		DBSingleton::instance().getDBObjectInstance()->insertGrDPStorageCacheElement(
				delegationId,
				dn,
				req,
				string(keytxt),
				string()
			);
	}

	deleg__NewProxyReq* ret = soap_new_deleg__NewProxyReq(ctx, -1);
	ret->proxyRequest = soap_new_std__string(ctx, -1);
	*ret->proxyRequest = req;
	ret->delegationID = soap_new_std__string(ctx, -1);
	*ret->delegationID = delegationId;

	if (reqtxt) free (reqtxt);
	if (keytxt) free (keytxt);

	return ret;
}

string GSoapDelegationHandler::x509ToString(X509* cert) {

	string str;
	BIO  *certmem;
	char *ptr = 0;

	certmem = BIO_new(BIO_s_mem());
	if (PEM_write_bio_X509(certmem, cert) == 1) {
		size_t len = BIO_get_mem_data(certmem, &ptr);
		str = string(ptr, len);
	}
    BIO_free(certmem);

    return str;
}

string GSoapDelegationHandler::addKeyToProxyCertificate(string proxy, string key) {

	stringstream ss;
	STACK_OF(X509) *certstack;

	if (GRSTx509StringToChain(&certstack, const_cast<char*>(proxy.c_str())) != GRST_RET_OK) {
		throw string("Failed to add private key to the proxy certificate!");
	}

	// first cert
	X509 *cert = sk_X509_value(certstack, 0);
	ss << x509ToString(cert);

	// private key
	ss << key;

	// other certs
	for (int i = 1; i < sk_X509_num(certstack); i++) {
		cert = sk_X509_value(certstack, i);
		ss << x509ToString(cert);
	}

	sk_X509_free(certstack);

	return ss.str();
}

time_t GSoapDelegationHandler::readTerminationTime(string proxy) {

	BIO *bio = BIO_new(BIO_s_mem());
	BIO_puts(bio, const_cast<char*>(proxy.c_str()));
	X509 *cert = PEM_read_bio_X509(bio, NULL, NULL, NULL);

	BIO_free(bio);
	if(!cert) throw string("Failed to determine proxy's termination time!");
    return GRSTasn1TimeToTimeT( (char*) ASN1_STRING_data(X509_get_notAfter(cert)), 0);
}

string GSoapDelegationHandler::getVomsAttributes(string proxy) {

	STACK_OF(X509) *certstack;

	if (GRSTx509StringToChain(&certstack, const_cast<char*>(proxy.c_str())) != GRST_RET_OK) {
		throw string("Failed to retrieve voms attributes from proxy certificate!");
	}

	X509 *cert = sk_X509_value(certstack, 0);

	vomsdata vdata;
	if(!vdata.Retrieve(cert, certstack)) {
		sk_X509_free(certstack);
		throw string("Failed to retrieve voms attributes from proxy certificate!");
	}

	static const string delimiter(" ");
	stringstream ss;

	vector<voms> userVoms = vdata.data;
	vector <voms>::iterator voms_it;
	vector<string> fqan;
	vector<string>::iterator fqan_it;

	for (voms_it = userVoms.begin(); voms_it != userVoms.end(); voms_it++) {

		fqan = voms_it->fqan;
		for (fqan_it = fqan.begin(); fqan_it < fqan.end(); fqan_it++) {
			ss << *fqan_it << delimiter;
		}
	}

	// get the last voms attribute;
	voms_it = userVoms.end() - 1;
	fqan_it = voms_it->fqan.end() - 1;
	attr = *fqan_it;

	sk_X509_free(certstack);
	return ss.str();
}

void GSoapDelegationHandler::putProxy(string delegationId, string proxy) {

	string dn = getDn();
	if (dn.empty()) throw string("'getDn' failed!");

	delegationId = handleDelegationId(delegationId);
	if (delegationId.empty()) throw string("'handleDelegationId' failed!");

	time_t time = readTerminationTime(proxy);
	string attrs = getVomsAttributes(proxy);

	string key;
	CredCache* cache = DBSingleton::instance().getDBObjectInstance()->findGrDPStorageCacheElement(delegationId, dn);
	if (cache) {
		key = cache->privateKey;
		delete cache;
	} else
		throw string("Failed to retrieve the cache element from DB!");

	proxy = addKeyToProxyCertificate(proxy, key);

	// create a new delegation ID using voms attributes!
	// has to be called after getVomsAttributes!!!
	delegationId = makeDelegationId();

	Cred* cred = DBSingleton::instance().getDBObjectInstance()->findGrDPStorageElement(delegationId, dn);
	if (cred) {
		DBSingleton::instance().getDBObjectInstance()->updateGrDPStorageElement(
				delegationId,
				dn,
				proxy,
				attrs,
				time
			);
		delete cred;

	} else {
		DBSingleton::instance().getDBObjectInstance()->insertGrDPStorageElement(
				delegationId,
				dn,
				proxy,
				attrs,
				time
			);
	}
}

string GSoapDelegationHandler::renewProxyReq(string delegationId) {

	string dn = getDn();
	if (dn.empty()) throw string("'getDn' failed!");

	if (delegationId.empty()) {
		throw string("Delegation ID is empty!");
	}

	if (!checkDelegationId(delegationId)) {
		throw string("Wrong format of delegation ID!");
	}

	char *reqtxt = 0, *keytxt = 0;
	int err = GRSTx509CreateProxyRequest(&reqtxt, &keytxt, 0);

	if (err) {
		if (reqtxt) free (reqtxt);
		if (keytxt) free (keytxt);
		throw string("'GRSTx509CreateProxyRequest' failed!");
	}

	string req (reqtxt);

	CredCache* cache = DBSingleton::instance().getDBObjectInstance()->findGrDPStorageCacheElement(delegationId, dn);
	if (cache) {
		DBSingleton::instance().getDBObjectInstance()->updateGrDPStorageCacheElement(
				delegationId,
				dn,
				req,
				string(keytxt),
				string()
			);
		delete cache;

	} else {
		DBSingleton::instance().getDBObjectInstance()->insertGrDPStorageCacheElement(
				delegationId,
				dn,
				req,
				string(keytxt),
				string()
			);
	}

	if (reqtxt) free (reqtxt);
	if (keytxt) free (keytxt);

	return req;
}

time_t GSoapDelegationHandler::getTerminationTime(string delegationId) {

	string dn = getDn();
	if (dn.empty()) throw string("'getDn' failed!");

	delegationId = handleDelegationId(delegationId);
	if (delegationId.empty()) throw string("'handleDelegationId' failed!");

	//delegationId = makeDelegationId(); // should return always the same delegation ID for the same user

	time_t time;
	Cred* cred = DBSingleton::instance().getDBObjectInstance()->findGrDPStorageElement(delegationId, dn);
	if (cred) {

		time = cred->termination_time;
		delete cred;

	} else
		throw string("Failed to retrieve storage element from DB!");

	return time;
}

void GSoapDelegationHandler::destroy(string delegationId) {

	string dn = getDn();
	if (dn.empty()) throw string("'getDn' failed!");

	delegationId = handleDelegationId(delegationId);
	if (delegationId.empty()) throw string("'handleDelegationId' failed!");

	DBSingleton::instance().getDBObjectInstance()->deleteGrDPStorageCacheElement(delegationId, dn);
	DBSingleton::instance().getDBObjectInstance()->deleteGrDPStorageElement(delegationId, dn);
}

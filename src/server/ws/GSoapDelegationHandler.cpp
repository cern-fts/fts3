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
 *
 * GSoapDelegationHandler.cpp
 *
 *  Created on: -- --, 2012
 *      Author: Michał Simon
 */

#include "GSoapDelegationHandler.h"

#include "db/generic/SingleDbInstance.h"

#include "common/logger.h"
#include <common/error.h>

#include <cgsi_plugin.h>
#include <stdlib.h>

#include <boost/regex.hpp>
#include <sstream>

using namespace fts3::ws;
using namespace db;
using namespace boost;

GSoapDelegationHandler::GSoapDelegationHandler(soap* ctx): ctx(ctx) {

	// get client DN
	char buff[200];
	int len = 200;
	if (get_client_dn(ctx, buff, len)) throw Err_Custom("'get_client_dn' failed!"); // if there's an error throw an exception
	dn = buff;

	// retrieve VOMS attributes (fqnas)
	int nbfqans = 0;
	char **arr = get_client_roles(ctx, &nbfqans);

	for (int i = 0; i < nbfqans; i++) {
		attrs.push_back(arr[i]);
	}
}

GSoapDelegationHandler::~GSoapDelegationHandler() {

}

string GSoapDelegationHandler::getClientDn(){
	std::cerr << dn << std::endl;
	return dn;
}

string GSoapDelegationHandler::getClientVo() {

	char* vo  = get_client_voname(ctx);

	if (vo) {
		return string(vo);
	}

	return string();
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
	EVP_DigestUpdate(&ctx, dn.c_str(), dn.size());

	// use last voms attribute if available!
	if (!attrs.empty()) {
		vector<string>::iterator fqan = attrs.end() - 1;
		EVP_DigestUpdate(&ctx, fqan->c_str(), fqan->size());
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

	delegationId = handleDelegationId(delegationId);
	if (delegationId.empty()) throw Err_Custom("'handleDelegationId' failed!");

	char *reqtxt = 0, *keytxt = 0;
	int err = GRSTx509CreateProxyRequest(&reqtxt, &keytxt, 0);

	if (err) {
		if (reqtxt) free (reqtxt);
		if (keytxt) free (keytxt);
		throw Err_Custom("'GRSTx509CreateProxyRequest' failed!");
	}

	string req (reqtxt);

	CredCache* cache = DBSingleton::instance().getDBObjectInstance()->findGrDPStorageCacheElement(delegationId, dn);
	if (cache) {
		DBSingleton::instance().getDBObjectInstance()->updateGrDPStorageCacheElement(
				delegationId,
				dn,
				req,
				string(keytxt),
				fqansToString(attrs)
			);
		delete cache;

	} else {
		DBSingleton::instance().getDBObjectInstance()->insertGrDPStorageCacheElement(
				delegationId,
				dn,
				req,
				string(keytxt),
				fqansToString(attrs)
			);
	}

	if (reqtxt) free (reqtxt);
	if (keytxt) free (keytxt);

	return req;
}

delegation__NewProxyReq* GSoapDelegationHandler::getNewProxyReq() {

	string delegationId = makeDelegationId();
	if (delegationId.empty()) throw Err_Custom("'getDelegationId' failed!");

	char *reqtxt = 0, *keytxt = 0;
	int err = GRSTx509CreateProxyRequest(&reqtxt, &keytxt, 0);

	if (err) {
		if (reqtxt) free (reqtxt);
		if (keytxt) free (keytxt);
		throw Err_Custom("'GRSTx509CreateProxyRequest' failed!");
	}

	string req (reqtxt);

	CredCache* cache = DBSingleton::instance().getDBObjectInstance()->findGrDPStorageCacheElement(delegationId, dn);
	if (cache) {
		DBSingleton::instance().getDBObjectInstance()->updateGrDPStorageCacheElement(
				delegationId,
				dn,
				req,
				string(keytxt),
				fqansToString(attrs)
			);
		delete cache;

	} else {
		DBSingleton::instance().getDBObjectInstance()->insertGrDPStorageCacheElement(
				delegationId,
				dn,
				req,
				string(keytxt),
				fqansToString(attrs)
			);
	}

	delegation__NewProxyReq* ret = soap_new_delegation__NewProxyReq(ctx, -1);
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
		throw Err_Custom("Failed to add private key to the proxy certificate!");
	}

	// first cert
	X509 *cert = sk_X509_value(certstack, 0);
	ss << x509ToString(cert);
	X509_free(cert);

	// private key
	ss << key;

	// other certs
	for (int i = 1; i < sk_X509_num(certstack); i++) {
		cert = sk_X509_value(certstack, i);
		ss << x509ToString(cert);
		X509_free(cert);
	}

	sk_X509_free(certstack);

	return ss.str();
}

time_t GSoapDelegationHandler::readTerminationTime(string proxy) {

	BIO *bio = BIO_new(BIO_s_mem());
	BIO_puts(bio, const_cast<char*>(proxy.c_str()));
	X509 *cert = PEM_read_bio_X509(bio, NULL, NULL, NULL);
	BIO_free(bio);

	if(!cert) throw Err_Custom("Failed to determine proxy's termination time!");

	time_t time = GRSTasn1TimeToTimeT( (char*) ASN1_STRING_data(X509_get_notAfter(cert)), 0);
	X509_free(cert);

    return time;
}

string GSoapDelegationHandler::fqansToString(vector<string> attrs) {

	stringstream ss;
	const string delimiter = " ";

	vector<string>::iterator it;
	for (it = attrs.begin(); it < attrs.end(); it++) {
		ss << *it << delimiter;
	}

	return ss.str();
}

void GSoapDelegationHandler::putProxy(string delegationId, string proxy) {

	delegationId = handleDelegationId(delegationId);
	if (delegationId.empty()) throw Err_Custom("'handleDelegationId' failed!");

	time_t time = readTerminationTime(proxy);

	string key;
	CredCache* cache = DBSingleton::instance().getDBObjectInstance()->findGrDPStorageCacheElement(delegationId, dn);
	if (cache) {
		key = cache->privateKey;
		delete cache;
	} else
		throw Err_Custom("Failed to retrieve the cache element from DB!");

	proxy = addKeyToProxyCertificate(proxy, key);

	Cred* cred = DBSingleton::instance().getDBObjectInstance()->findGrDPStorageElement(delegationId, dn);
	if (cred) {
		DBSingleton::instance().getDBObjectInstance()->updateGrDPStorageElement(
				delegationId,
				dn,
				proxy,
				fqansToString(attrs),
				time
			);
		delete cred;

	} else {
		DBSingleton::instance().getDBObjectInstance()->insertGrDPStorageElement(
				delegationId,
				dn,
				proxy,
				fqansToString(attrs),
				time
			);
	}
}

string GSoapDelegationHandler::renewProxyReq(string delegationId) {

	// it is different to gridsite implementation but it is done like that in delegation-java
	// in GliteDelegation.java
	delegationId = handleDelegationId(delegationId);
	if (delegationId.empty()) throw Err_Custom("'handleDelegationId' failed!");

	if (!checkDelegationId(delegationId)) {
		throw Err_Custom("Wrong format of delegation ID!");
	}

	char *reqtxt = 0, *keytxt = 0;
	int err = GRSTx509CreateProxyRequest(&reqtxt, &keytxt, 0);

	if (err) {
		if (reqtxt) free (reqtxt);
		if (keytxt) free (keytxt);
		throw Err_Custom("'GRSTx509CreateProxyRequest' failed!");
	}

	string req (reqtxt);

	CredCache* cache = DBSingleton::instance().getDBObjectInstance()->findGrDPStorageCacheElement(delegationId, dn);
	if (cache) {
		DBSingleton::instance().getDBObjectInstance()->updateGrDPStorageCacheElement(
				delegationId,
				dn,
				req,
				string(keytxt),
				fqansToString(attrs)
			);
		delete cache;

	} else {
		DBSingleton::instance().getDBObjectInstance()->insertGrDPStorageCacheElement(
				delegationId,
				dn,
				req,
				string(keytxt),
				fqansToString(attrs)
			);
	}

	if (reqtxt) free (reqtxt);
	if (keytxt) free (keytxt);

	return req;
}

time_t GSoapDelegationHandler::getTerminationTime(string delegationId) {

	delegationId = makeDelegationId(); // should return always the same delegation ID for the same user

	time_t time;
	Cred* cred = DBSingleton::instance().getDBObjectInstance()->findGrDPStorageElement(delegationId, dn);
	if (cred) {

		time = cred->termination_time;
		delete cred;

	} else
		throw Err_Custom("Failed to retrieve storage element from DB!");

	return time;
}

void GSoapDelegationHandler::destroy(string delegationId) {

	delegationId = handleDelegationId(delegationId);
	if (delegationId.empty()) throw Err_Custom("'handleDelegationId' failed!");

	DBSingleton::instance().getDBObjectInstance()->deleteGrDPStorageCacheElement(delegationId, dn);
	DBSingleton::instance().getDBObjectInstance()->deleteGrDPStorageElement(delegationId, dn);
}

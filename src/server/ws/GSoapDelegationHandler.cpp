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

#include <gridsite.h>
#include <cgsi_plugin.h>
#include <stdlib.h>

#include <memory>
#include <sstream>
#include <boost/regex.hpp>


using namespace fts3::ws;
using namespace boost;

const string GSoapDelegationHandler::GRST_PROXYCACHE = "/../proxycache/";

GSoapDelegationHandler::GSoapDelegationHandler(soap* ctx): ctx(ctx) {

}

GSoapDelegationHandler::~GSoapDelegationHandler() {

}

// TODO !!!
string GSoapDelegationHandler::getDn() {

	char buff[201];
	static int len = 200;
	if (get_client_dn(ctx, buff, len)) throw string("'get_client_dn' failed!");
	return string(buff);
}

string GSoapDelegationHandler::getDelegationId() {

	char* delegation_id = GRSTx509MakeDelegationID();
	string ret = delegation_id;
	free(delegation_id); // TODO ??? do we need to free this memory, in grst-delegation they don't
	return ret;
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

		return getDelegationId();
	}

	if (!checkDelegationId(delegationId)) return string();

	return delegationId;
}

string GSoapDelegationHandler::getProxyDirectory() {

	stringstream ss;
	string docroot = getenv("DOCUMENT_ROOT");
	ss << docroot << GRST_PROXYCACHE;
	return ss.str();
}

string GSoapDelegationHandler::getProxyReq(string delegationId) {

	string dn = getDn();
	if (dn.empty()) throw string("'getDn' failed!");

	delegationId = handleDelegationId(delegationId);
	if (delegationId.empty()) throw string("'handleDelegationId' failed!");

	string proxydir = getProxyDirectory();

	char* resp;
	int err = GRSTx509MakeProxyRequest(
			&resp,
			const_cast<char*>(proxydir.c_str()),
			const_cast<char*>(delegationId.c_str()),
			const_cast<char*>(dn.c_str())
		);

	if (err) {
		throw string("'GRSTx509MakeProxyRequest' failed!");
	}

	return string(resp);
}

deleg__NewProxyReq* GSoapDelegationHandler::getNewProxyReq() {

	string dn = getDn();
	if (dn.empty()) throw string("'getDn' failed!");

	string delegationId = getDelegationId();
	if (delegationId.empty()) throw string("'getDelegationId' failed!");

	string proxydir = getProxyDirectory();

	char* resp;
	int err = GRSTx509MakeProxyRequest(
			&resp,
			const_cast<char*>(proxydir.c_str()),
			const_cast<char*>(delegationId.c_str()),
			const_cast<char*>(dn.c_str())
		);

	if (err) {
		throw string("'GRSTx509MakeProxyRequest' failed!");
	}

	deleg__NewProxyReq* ret = soap_new_deleg__NewProxyReq(ctx, -1);
	ret->proxyRequest = soap_new_std__string(ctx, -1);
	*ret->proxyRequest = resp;
	ret->delegationID = soap_new_std__string(ctx, -1);
	*ret->delegationID = delegationId;

	return ret;
}

void GSoapDelegationHandler::putProxy(string delegationId, string proxy) {

	string dn = getDn();
	if (dn.empty()) throw string("'getDn' failed!");

	delegationId = handleDelegationId(delegationId);
	if (delegationId.empty()) throw string("'handleDelegationId' failed!");

	string proxydir = getProxyDirectory();

	int err = GRSTx509CacheProxy(
			const_cast<char*>(proxydir.c_str()),
			const_cast<char*>(delegationId.c_str()),
			const_cast<char*>(dn.c_str()),
			const_cast<char*>(proxy.c_str())
		);

	if (err) {
		throw string("'GRSTx509CacheProxy' failed!");
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

	string proxydir = getProxyDirectory();

	char* resp;
	int err = GRSTx509MakeProxyRequest(
			&resp,
			const_cast<char*>(proxydir.c_str()),
			const_cast<char*>(delegationId.c_str()),
			const_cast<char*>(dn.c_str())
		);

	if (err) {
		throw string("'GRSTx509MakeProxyRequest' failed!");
	}

	return string (resp);
}

time_t GSoapDelegationHandler::getTerminationTime(string delegationId) {

	string dn = getDn();
	if (dn.empty()) throw string("'getDn' failed!");

	delegationId = getDelegationId(); // should return always the same delegation ID for the same user

	string proxydir = getProxyDirectory();

	time_t start, finish;
	int err = GRSTx509ProxyGetTimes(
			const_cast<char*>(proxydir.c_str()),
			const_cast<char*>(delegationId.c_str()),
			const_cast<char*>(dn.c_str()),
            &start,
            &finish
		);

	if (err) {
		throw string("'GRSTx509ProxyGetTimes' failed!");
	}

	return finish;
}

void GSoapDelegationHandler::destroy(string delegationId) {

	string dn = getDn();
	if (dn.empty()) throw string("'getDn' failed!");

	delegationId = handleDelegationId(delegationId);
	if (delegationId.empty()) throw string("'handleDelegationId' failed!");

	string proxydir = getProxyDirectory();

	int err = GRSTx509ProxyDestroy(
			const_cast<char*>(proxydir.c_str()),
			const_cast<char*>(delegationId.c_str()),
			const_cast<char*>(dn.c_str())
		);

	if (err) {
		throw string("'GRSTx509ProxyDestroy' failed!");
	}
}

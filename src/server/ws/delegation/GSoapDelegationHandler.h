/*
 * Copyright (c) CERN 2013-2015
 *
 * Copyright (c) Members of the EMI Collaboration. 2010-2013
 *  See  http://www.eu-emi.eu/partners for details on the copyright
 *  holders.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef GSOAPDELEGATIONHANDLER_H_
#define GSOAPDELEGATIONHANDLER_H_


#include "DelegationRequestCache.h"

#include <string>
#include <gridsite.h>

#include "ws-ifce/gsoap/gsoap_stubs.h"

namespace fts3
{
namespace ws
{

using namespace std;

/**
 * The GSoapDelegationHandler class is responsible for proxy certificate delegation on the server side
 *
 * The delegation protocol implies that in the first step the client sends a getProxy request, the server
 * generates a public and private key pair, and sends the public key to the client (it is signed using
 * server certificate). Subsequently, the client signs this request using its own private key and certificate,
 * and sends a putProxy message back to the server, containing the signed certificate.
 *
 * CGSI_GSoap provides https functionality, and is used to retrieve client DN and VOMS attributes (FQAN)
 *
 * The delegationId that identifies a proxy certificate is created based on used DN and VOMS attributes (FQANs),
 * therefore event if the delegation ID is not specified it is possible to recreate it!
 */
class GSoapDelegationHandler
{
public:

    static void init();

    /**
     * Constructor
     *
     * Retrieves client DN and VOMS attributes using CGSI-GSoap
     */
    GSoapDelegationHandler(soap* ctx);

    /**
     * Destructor
     */
    virtual ~GSoapDelegationHandler();

    /**
     * Creates a unique delegation ID based on the client DN and VOMS attributes
     *
     * @return unique delegation ID, it will be always the same
     * 			for the same user and VOMS attributes
     */
    string makeDelegationId();

    /**
     * If the delegation ID is empty creates a new one using
     * 'makeDelegationId', otherwise checks if the format is correct.
     *
     * @param delegationId - the delegation ID provide by client
     *
     * @return delegation ID, or empty string if the format was not correct
     *
     * @see makeDelegationId
     */
    string handleDelegationId(string delegationId);

    /**
     * Checks if the delegation ID format is right
     *
     * @param delegationId - the delegation ID that is being checked
     *
     * @return true if the format is right, false otherwise
     */
    bool checkDelegationId(string delegationId);

    /**
     * Converts a X509 struct to string
     *
     * @param cert - the X509 struct that has to be converted
     *
     * @return a string containing the certificate
     */
    string x509ToString(X509* cert);

    /**
     * Adds the private (the one created in the getProxy step) to the proxy certificate.
     * The is added at the second place in the certificate after the proxy itself.
     *
     * @param proxy - the proxy certificate delegated by the client
     * @param key - the private key that has to be added to the certificate
     *
     * @return a string containing the certificate with the key (one the 2nd possition)
     */
    string addKeyToProxyCertificate(string proxy, string key);

    /**
     * Reads the expiration time from a proxy certificate
     *
     * @param proxy - the proxy certificate
     *
     * @return expiration time
     */
    time_t readTerminationTime(string proxy);

    /**
     * Converts a vector containing VOMS attributes (FQANs) to string
     * Uses space as a delimiter.
     *
     * @param attrs - vector containing VOMS attributes
     *
     * @return string containing all VOMS attributes (space is used a a delimiter)
     */
    string fqansToString(vector<string> attrs);

    /**
     * The WebServer getProxy request method.
     *
     * Creates a public & private key pair, the
     * private key is stored in the DB, the public
     * key is sent to the client
     *
     * @param delegationId - the delegation ID specified by the client,
     * 							an empty string is also accepted
     *
     * 	@return string containing the public key
     */
    string getProxyReq(string delegationId);

    /**
     * The WebServer renewProxy request method.
     *
     * @param delegationId - the delegation ID specified by the client,
     * 							an empty string is also accepted
     *
     * @return string containing a new public key
     */
    string renewProxyReq(string delegationId);

    /**
     * The WebServer getTerminationTime request method.
     *
     * Gets the expiration time of respective proxy certificate
     * (identified by the delegation ID)
     *
     * @param delegationId - NOT used (implemented in the same way as it is in gridsite)
     * 						the delegationId is generated inside based on client DN and
     * 						VOMS attributes
     *
     * @return expiration time of the respective proxy certificate
     */
    time_t getTerminationTime(string delegationId);

    /**
     * The WebServer getNewProxy request method.
     *
     * Generates a new delegation ID, then
     * creates a public & private key pair, the
     * private key is stored in the DB, the public
     * key is sent to the client
     *
     * 	@return delegation__NewProxyReq struct containing
     * 			the public key and the delegation ID
     */
    delegation__NewProxyReq* getNewProxyReq();

    /**
     * The WebServer putProxy request method.
     *
     * Adds the retrieves the respective private key from the DB,
     * and adds it to the proxy certificate. Subsequently, the
     * proxy certificate is stored in the DB for later use.
     *
     * @param delegationId - the delegation ID specified by the client,
     * 							an empty string is also accepted
     * @param proxy - the delegated proxy certificate
     */
    void putProxy(string delegationId, string proxy);

    /**
     *	The WebServer destroy request method.
     *
     *	Removes the respective proxy certificate from the DB.
     *
     * @param delegationId - the delegation ID specified by the client,
     * 							an empty string is also accepted
     */
    void destroy(string delegationId);

private:
    /// gSOAP context
    soap* ctx;

    /// client DN
    string dn;

    /// client VOMS attributes (FQAN)
    vector<string> attrs;
};

}
}

#endif /* GSOAPDELEGATIONHANDLER_H_ */

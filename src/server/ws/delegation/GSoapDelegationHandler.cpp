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
#include "ws/CGsiAdapter.h"

#include "db/generic/SingleDbInstance.h"

#include "common/logger.h"
#include "common/error.h"

#include <stdlib.h>

#include <boost/regex.hpp>
#include <boost/scoped_ptr.hpp>
#include <sstream>

using namespace fts3::ws;
using namespace db;
using namespace boost;


void GSoapDelegationHandler::init()
{
}

GSoapDelegationHandler::GSoapDelegationHandler(soap* ctx): ctx(ctx)
{

    CGsiAdapter cgsi(ctx);
    dn = cgsi.getClientDn();
    attrs = cgsi.getClientAttributes();
}

GSoapDelegationHandler::~GSoapDelegationHandler()
{

}

string GSoapDelegationHandler::makeDelegationId()
{

    unsigned char hash_delegation_id[EVP_MAX_MD_SIZE] = {0};
    unsigned int delegation_id_len = 0;
    char delegation_id[17] = {0};

    const EVP_MD *m = EVP_sha1();
    if (m == NULL) return NULL;

    EVP_MD_CTX *ctx =  EVP_MD_CTX_create();
    if(ctx == NULL)
	return NULL;

    if(1 != EVP_DigestInit_ex(ctx, m, NULL))
    {
        EVP_MD_CTX_destroy(ctx);
	return NULL;
    }
    // use DN
    if(1 != EVP_DigestUpdate(ctx, dn.c_str(), dn.size()))
    {
	EVP_MD_CTX_destroy(ctx);
	return NULL;
    }

    // use last voms attribute if available!
    if (!attrs.empty())
        {
            for (std::vector<std::string>::iterator fqan = attrs.begin(); fqan != attrs.end(); ++fqan)
                {
                    const void *d = fqan->c_str();
                    if(d)
                    {
                        if(1 != EVP_DigestUpdate(ctx, d, fqan->size()))
                        {
				EVP_MD_CTX_destroy(ctx);
				return NULL;
                        }
                    }
                }
        }

    if(1 != EVP_DigestFinal_ex(ctx, hash_delegation_id, &delegation_id_len))
    {
	EVP_MD_CTX_destroy(ctx);
	return NULL;
    }

    for (int i = 0; i < 8; i++)
        sprintf(&delegation_id[i*2], "%02x", hash_delegation_id[i]);

    delegation_id[16] = '\0';

    EVP_MD_CTX_destroy(ctx);

    return delegation_id;
}

bool GSoapDelegationHandler::checkDelegationId(string delegationId)
{

    static string exp = "^[a-zA-Z0-9\\.,_]*$"; // only alphanumeric characters + '.', ',' and '_'
    static regex re (exp);

    smatch what;
    regex_match(delegationId, what, re, match_extra);

    return !string(what[0]).empty();
}

string GSoapDelegationHandler::handleDelegationId(string delegationId)
{

    if (delegationId.empty())
        {
            return makeDelegationId();
        }

    if (!checkDelegationId(delegationId)) return string();

    return delegationId;
}

string GSoapDelegationHandler::getProxyReq(string delegationId)
{

    FTS3_COMMON_LOGGER_NEWLOG (INFO) << "DN: " << dn << " gets proxy certificate request" << commit;

    delegationId = handleDelegationId(delegationId);
    if (delegationId.empty()) throw Err_Custom("'handleDelegationId' failed!");

    // check if the public - private key pair has been already generated and is in the DB cache
    scoped_ptr<CredCache> cache (
        DBSingleton::instance().getDBObjectInstance()->findGrDPStorageCacheElement(delegationId, dn)
    );

    if (cache.get())
        {
            FTS3_COMMON_LOGGER_NEWLOG (INFO) << "DN: " << dn << " public-private key pair has been found in DB and is returned to the user" << commit;
            return cache->certificateRequest;
        }

    char *reqtxt = NULL, *keytxt = NULL;
    int err = GRSTx509CreateProxyRequest(&reqtxt, &keytxt, 0);

    if (err)
        {
            if (reqtxt) free(reqtxt);
            if (keytxt) free(keytxt);
            throw Err_Custom("'GRSTx509CreateProxyRequest' failed!");
        }

    string req (reqtxt);

    try
        {
            bool inserted = DBSingleton::instance().getDBObjectInstance()->insertGrDPStorageCacheElement(
                                delegationId,
                                dn,
                                req,
                                keytxt,
                                fqansToString(attrs)
                            );

            if (!inserted)
                {
                    // double check if the public - private key pair has been already generated and is in the DB cache
                    cache.reset(
                        DBSingleton::instance().getDBObjectInstance()->findGrDPStorageCacheElement(delegationId, dn)
                    );

                    if (cache.get())
                        {
                            if (reqtxt) free(reqtxt);
                            if (keytxt) free(keytxt);
                            FTS3_COMMON_LOGGER_NEWLOG (INFO) << "DN: " << dn << " public-private key pair has been found in DB and is returned to the user" << commit;
                            return cache->certificateRequest;
                        }

                    throw Err_Custom("Failed to insert the 'public-private' key into t_credential_cache!");
                }
        }
    catch(Err& ex)
        {
            if (reqtxt) free(reqtxt);
            if (keytxt) free(keytxt);
            throw Err_Custom(ex.what());

        }
    catch(...)
        {
            if (reqtxt) free(reqtxt);
            if (keytxt) free(keytxt);
            throw Err_Custom("Problem while renewing proxy");
        }

    if (reqtxt) free(reqtxt);
    if (keytxt) free(keytxt);

    FTS3_COMMON_LOGGER_NEWLOG (INFO) << "DN: " << dn << " new public-private key pair has been generated and returned to the user" << commit;

    return req;
}

delegation__NewProxyReq* GSoapDelegationHandler::getNewProxyReq()
{

    FTS3_COMMON_LOGGER_NEWLOG (INFO) << "DN: " << dn << " gets new proxy certificate request" << commit;

    string delegationId = makeDelegationId();
    if (delegationId.empty()) throw Err_Custom("'getDelegationId' failed!");

    scoped_ptr<CredCache> cache (
        DBSingleton::instance().getDBObjectInstance()->findGrDPStorageCacheElement(delegationId, dn)
    );

    if (cache.get())
        {
            delegation__NewProxyReq* ret = soap_new_delegation__NewProxyReq(ctx, -1);
            ret->proxyRequest = soap_new_std__string(ctx, -1);
            *ret->proxyRequest = cache->certificateRequest;
            ret->delegationID = soap_new_std__string(ctx, -1);
            *ret->delegationID = delegationId;
            return ret;
        }

    char *reqtxt = 0, *keytxt = 0;
    int err = GRSTx509CreateProxyRequest(&reqtxt, &keytxt, 0);

    if (err)
        {
            if (reqtxt) free (reqtxt);
            if (keytxt) free (keytxt);
            throw Err_Custom("'GRSTx509CreateProxyRequest' failed!");
        }

    string req (reqtxt);

    try
        {
            DBSingleton::instance().getDBObjectInstance()->insertGrDPStorageCacheElement(
                delegationId,
                dn,
                req,
                keytxt,
                fqansToString(attrs)
            );
        }
    catch(Err& ex)
        {
            if (reqtxt) free (reqtxt);
            if (keytxt) free (keytxt);
            throw Err_Custom(ex.what());
        }
    catch(...)
        {
            if (reqtxt) free (reqtxt);
            if (keytxt) free (keytxt);
            throw Err_Custom("Problem while renewing proxy");
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

string GSoapDelegationHandler::x509ToString(X509* cert)
{

    string str;
    BIO  *certmem = NULL;
    char *ptr = NULL;

    certmem = BIO_new(BIO_s_mem());
    if (PEM_write_bio_X509(certmem, cert) == 1)
        {
            size_t len = BIO_get_mem_data(certmem, &ptr);
            str = string(ptr, len);
        }
    BIO_free(certmem);

    return str;
}

string GSoapDelegationHandler::addKeyToProxyCertificate(string proxy, string key)
{

    // first check if the key matches the certificate
    // (it can happen in case of a run condition -
    // two clients are delegating the proxy concurrently)

    BIO *bio = BIO_new(BIO_s_mem());
    BIO_puts(bio, const_cast<char*>(key.c_str()));
    EVP_PKEY* prvKey = PEM_read_bio_PrivateKey(bio, 0, 0, 0);
    BIO_free(bio);

    bio = BIO_new(BIO_s_mem());
    BIO_puts(bio, const_cast<char*>(proxy.c_str()));
    X509* cert = PEM_read_bio_X509(bio, 0, 0, 0);
    BIO_free(bio);

    bool mismatch = !X509_check_private_key(cert, prvKey);
    X509_free(cert);
    EVP_PKEY_free(prvKey);

    // if the private key does not match throw an exception
    if (mismatch)
        {
            throw Err_Transient("Failed to add private key to the proxy certificate: key values mismatch!");
        }

    stringstream ss;
    STACK_OF(X509) *certstack;

    if (GRSTx509StringToChain(&certstack, const_cast<char*>(proxy.c_str())) != GRST_RET_OK)
        {
            throw Err_Custom("Failed to add private key to the proxy certificate!");
        }

    // first cert
    cert = sk_X509_value(certstack, 0);
    ss << x509ToString(cert);
    X509_free(cert);

    // private key
    ss << key;

    // other certs
    for (int i = 1; i < sk_X509_num(certstack); i++)
        {
            cert = sk_X509_value(certstack, i);
            ss << x509ToString(cert);
            X509_free(cert);
        }

    sk_X509_free(certstack);

    return ss.str();
}

time_t GSoapDelegationHandler::readTerminationTime(string proxy)
{

    BIO *bio = BIO_new(BIO_s_mem());
    BIO_puts(bio, const_cast<char*>(proxy.c_str()));
    X509 *cert = PEM_read_bio_X509(bio, NULL, NULL, NULL);
    BIO_free(bio);

    if(!cert) throw Err_Custom("Failed to determine proxy's termination time!");

    time_t time = GRSTasn1TimeToTimeT( (char*) ASN1_STRING_data(X509_get_notAfter(cert)), 0);
    X509_free(cert);

    return time;
}

string GSoapDelegationHandler::fqansToString(vector<string> attrs)
{

    stringstream ss;
    const string delimiter = " ";

    vector<string>::iterator it;
    for (it = attrs.begin(); it < attrs.end(); ++it)
        {
            ss << *it << delimiter;
        }

    return ss.str();
}

void GSoapDelegationHandler::putProxy(string delegationId, string proxy)
{
    try
        {

            FTS3_COMMON_LOGGER_NEWLOG (INFO) << "DN: " << dn << " puts proxy certificate" << commit;

            delegationId = handleDelegationId(delegationId);
            if (delegationId.empty()) throw Err_Custom("'handleDelegationId' failed!");

            time_t incomingExpirationTime = readTerminationTime(proxy);

            // make sure it is valid for more than an hour
            time_t now = time(NULL);

            if (difftime(incomingExpirationTime, now) < 3600)
                {
                    throw Err_Custom("The proxy has to be valid for at least an hour!");
                }

            scoped_ptr<CredCache> cache (
                DBSingleton::instance().getDBObjectInstance()->findGrDPStorageCacheElement(delegationId, dn)
            );

            // if the DB cache is empty it means someone else
            // already delegated and cleared the cache
            if (!cache.get())
                {

                    FTS3_COMMON_LOGGER_NEWLOG (INFO) << "DN: " << dn << "t_credential_cache has been cleared - so there's nothing to do" << commit;
                    return;
                }

            proxy = addKeyToProxyCertificate(proxy, cache->privateKey);

            scoped_ptr<Cred> cred (
                DBSingleton::instance().getDBObjectInstance()->findGrDPStorageElement(delegationId, dn)
            );

            try
                {
                    if (cred.get())
                        {
                            // check if the termination time is better than for the current proxy (if not we don't bother)
                            if (incomingExpirationTime < cred->termination_time)
                                {
                                    // log the termination time of the current and the new proxy
                                    FTS3_COMMON_LOGGER_NEWLOG (INFO)
                                            << "Current proxy termination time: "
                                            << cred->termination_time
                                            << ", new proxy proxy termination time: "
                                            << incomingExpirationTime
                                            << " (the new proxy won't be used)"
                                            << commit;
                                    return;
                                }

                            DBSingleton::instance().getDBObjectInstance()->updateGrDPStorageElement(
                                delegationId,
                                dn,
                                proxy,
                                fqansToString(attrs),
                                incomingExpirationTime
                            );
                        }
                    else
                        {
                            DBSingleton::instance().getDBObjectInstance()->insertGrDPStorageElement(
                                delegationId,
                                dn,
                                proxy,
                                fqansToString(attrs),
                                incomingExpirationTime
                            );
                        }
                    FTS3_COMMON_LOGGER_NEWLOG (INFO) << "DN: " << dn << " new proxy is in t_credential" << commit;
                }
            catch(Err& ex)
                {
                    throw Err_Custom(ex.what());
                }
            catch(...)
                {
                    throw Err_Custom("Failed to put proxy certificate");
                }

            // clear the DB cache
            DBSingleton::instance().getDBObjectInstance()->deleteGrDPStorageCacheElement(delegationId, dn);
            FTS3_COMMON_LOGGER_NEWLOG (INFO) << "DN: " << dn << " t_credential_cache has been cleared" << commit;
        }
    catch(Err& ex)
        {
            throw Err_Custom(ex.what());
        }
    catch(...)
        {
            throw Err_Custom("Failed to put proxy certificate");
        }
}

string GSoapDelegationHandler::renewProxyReq(string delegationId)
{

    string req;
    try
        {
            FTS3_COMMON_LOGGER_NEWLOG (INFO) << "DN: " << dn << " renews proxy certificate" << commit;

            // it is different to gridsite implementation but it is done like that in delegation-java
            // in GliteDelegation.java
            delegationId = handleDelegationId(delegationId);
            if (delegationId.empty()) throw Err_Custom("'handleDelegationId' failed!");

            scoped_ptr<CredCache> cache (
                DBSingleton::instance().getDBObjectInstance()->findGrDPStorageCacheElement(delegationId, dn)
            );

            if(cache.get())
                {
                    return cache->certificateRequest;
                }

            char *reqtxt = NULL, *keytxt = NULL;
            int err = GRSTx509CreateProxyRequest(&reqtxt, &keytxt, 0);

            if (err)
                {
                    if (reqtxt) free (reqtxt);
                    if (keytxt) free (keytxt);
                    throw Err_Custom("'GRSTx509CreateProxyRequest' failed!");
                }

            req = std::string(reqtxt);

            try
                {
                    DBSingleton::instance().getDBObjectInstance()->insertGrDPStorageCacheElement(
                        delegationId,
                        dn,
                        req,
                        keytxt,
                        fqansToString(attrs)
                    );
                }
            catch(Err& ex)
                {
                    if (reqtxt) free (reqtxt);
                    if (keytxt) free (keytxt);
                    throw Err_Custom(ex.what());
                }
            catch(...)
                {
                    if (reqtxt) free (reqtxt);
                    if (keytxt) free (keytxt);
                    throw Err_Custom("Failed to renew proxy certificate");
                }

            if (reqtxt) free (reqtxt);
            if (keytxt) free (keytxt);
        }
    catch(Err& ex)
        {
            throw Err_Custom(ex.what());
        }
    catch(...)
        {
            throw Err_Custom("Failed to renewProxyReq proxy certificate");
        }
    return req;
}

time_t GSoapDelegationHandler::getTerminationTime(string delegationId)
{

    try
        {
            FTS3_COMMON_LOGGER_NEWLOG (INFO) << "DN: " << dn << " gets proxy certificate termination time" << commit;

            string delegationId = makeDelegationId();
            if (delegationId.empty()) throw Err_Custom("'getDelegationId' failed!");

            time_t time;
            scoped_ptr<Cred> cred (
                DBSingleton::instance().getDBObjectInstance()->findGrDPStorageElement(delegationId, dn)
            );

            if (cred.get())
                {
                    return cred->termination_time;
                }
            else
                {
                    throw Err_Custom("Failed to retrieve termination time for DN " + dn);
                }
        }
    catch(Err& ex)
        {
            throw Err_Custom(ex.what());
        }
    catch(...)
        {
            throw Err_Custom("Failed proxy getTerminationTime certificate");
        }
}

void GSoapDelegationHandler::destroy(string delegationId)
{

    FTS3_COMMON_LOGGER_NEWLOG (INFO) << "DN: " << dn << " destroys proxy certificate" << commit;

    delegationId = handleDelegationId(delegationId);
    if (delegationId.empty()) throw Err_Custom("'handleDelegationId' failed!");

    try
        {
            DBSingleton::instance().getDBObjectInstance()->deleteGrDPStorageCacheElement(delegationId, dn);
            DBSingleton::instance().getDBObjectInstance()->deleteGrDPStorageElement(delegationId, dn);
        }
    catch(Err& ex)
        {
            throw Err_Custom(ex.what());
        }
    catch(...)
        {
            throw Err_Custom("Failed to destroy proxy certificate");
        }
}

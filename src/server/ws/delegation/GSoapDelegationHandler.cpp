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

#include "GSoapDelegationHandler.h"
#include "ws/CGsiAdapter.h"

#include "db/generic/SingleDbInstance.h"

#include "common/logger.h"
#include "common/error.h"

#include <stdlib.h>

#include <boost/regex.hpp>
#include <sstream>

using namespace fts3::ws;
using namespace db;


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

std::string GSoapDelegationHandler::makeDelegationId()
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

bool GSoapDelegationHandler::checkDelegationId(std::string delegationId)
{

    static std::string exp = "^[a-zA-Z0-9\\.,_]*$"; // only alphanumeric characters + '.', ',' and '_'
    static boost::regex re (exp);

    boost::smatch what;
    boost::regex_match(delegationId, what, re, boost::match_extra);

    return !std::string(what[0]).empty();
}

std::string GSoapDelegationHandler::handleDelegationId(std::string delegationId)
{

    if (delegationId.empty())
        {
            return makeDelegationId();
        }

    if (!checkDelegationId(delegationId)) return std::string();

    return delegationId;
}

std::string GSoapDelegationHandler::getProxyReq(std::string delegationId)
{

    FTS3_COMMON_LOGGER_NEWLOG (INFO) << "DN: " << dn << " gets proxy certificate request" << commit;

    delegationId = handleDelegationId(delegationId);
    if (delegationId.empty()) throw Err_Custom("'handleDelegationId' failed!");

    // check if the public - private key pair has been already generated and is in the DB cache
    boost::optional<UserCredentialCache> cache (
        DBSingleton::instance().getDBObjectInstance()->findCredentialCache(delegationId, dn)
    );

    if (cache)
        {
            FTS3_COMMON_LOGGER_NEWLOG (INFO) << "DN: " << dn << " public-private key pair has been found in DB and is returned to the user" << commit;
            return cache->certificateRequest;
        }

    char *reqtxt = NULL, *keytxt = NULL;
    int err = GRSTx509CreateProxyRequest(&reqtxt, &keytxt, 0);

    if (err)
        {
            free(reqtxt);
            free(keytxt);
            throw Err_Custom("'GRSTx509CreateProxyRequest' failed!");
        }

    std::string req (reqtxt);

    try
        {
            bool inserted = DBSingleton::instance().getDBObjectInstance()->insertCredentialCache(
                                delegationId,
                                dn,
                                req,
                                keytxt,
                                fqansToString(attrs)
                            );

            if (!inserted)
                {
                    // double check if the public - private key pair has been already generated and is in the DB cache
                    cache = DBSingleton::instance().getDBObjectInstance()->findCredentialCache(delegationId, dn);

                    if (cache)
                        {
                            free(reqtxt);
                            free(keytxt);
                            FTS3_COMMON_LOGGER_NEWLOG (INFO) << "DN: " << dn << " public-private key pair has been found in DB and is returned to the user" << commit;
                            return cache->certificateRequest;
                        }

                    throw Err_Custom("Failed to insert the 'public-private' key into t_credential_cache!");
                }
        }
    catch(Err& ex)
        {
            free(reqtxt);
            free(keytxt);
            throw Err_Custom(ex.what());

        }
    catch(...)
        {
            free(reqtxt);
            free(keytxt);
            throw Err_Custom("Problem while renewing proxy");
        }

    free(reqtxt);
    free(keytxt);

    FTS3_COMMON_LOGGER_NEWLOG (INFO) << "DN: " << dn << " new public-private key pair has been generated and returned to the user" << commit;

    return req;
}

delegation__NewProxyReq* GSoapDelegationHandler::getNewProxyReq()
{

    FTS3_COMMON_LOGGER_NEWLOG (INFO) << "DN: " << dn << " gets new proxy certificate request" << commit;

    std::string delegationId = makeDelegationId();
    if (delegationId.empty()) throw Err_Custom("'getDelegationId' failed!");

    boost::optional<UserCredentialCache> cache (
        DBSingleton::instance().getDBObjectInstance()->findCredentialCache(delegationId, dn)
    );

    if (cache)
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

    std::string req (reqtxt);

    try
        {
            DBSingleton::instance().getDBObjectInstance()->insertCredentialCache(
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

std::string GSoapDelegationHandler::x509ToString(X509* cert)
{

    std::string str;
    BIO  *certmem = NULL;
    char *ptr = NULL;

    certmem = BIO_new(BIO_s_mem());
    if (PEM_write_bio_X509(certmem, cert) == 1)
        {
            size_t len = BIO_get_mem_data(certmem, &ptr);
            str = std::string(ptr, len);
        }
    BIO_free(certmem);

    return str;
}

std::string GSoapDelegationHandler::addKeyToProxyCertificate(std::string proxy, std::string key)
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

    std::stringstream ss;
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

time_t GSoapDelegationHandler::readTerminationTime(std::string proxy)
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

std::string GSoapDelegationHandler::fqansToString(std::vector<std::string> attrs)
{

    std::stringstream ss;
    const std::string delimiter = " ";

    std::vector<std::string>::iterator it;
    for (it = attrs.begin(); it < attrs.end(); ++it)
        {
            ss << *it << delimiter;
        }

    return ss.str();
}

void GSoapDelegationHandler::putProxy(std::string delegationId, std::string proxy)
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

            boost::optional<UserCredentialCache> cache (
                DBSingleton::instance().getDBObjectInstance()->findCredentialCache(delegationId, dn)
            );

            // if the DB cache is empty it means someone else
            // already delegated and cleared the cache
            if (!cache)
                {

                    FTS3_COMMON_LOGGER_NEWLOG (INFO) << "DN: " << dn << "t_credential_cache has been cleared - so there's nothing to do" << commit;
                    return;
                }

            proxy = addKeyToProxyCertificate(proxy, cache->privateKey);

            boost::optional<UserCredential> cred (
                DBSingleton::instance().getDBObjectInstance()->findCredential(delegationId, dn)
            );

            try
                {
                    if (cred)
                        {
                            // check if the termination time is better than for the current proxy (if not we don't bother)
                            if (incomingExpirationTime < cred->terminationTime)
                                {
                                    // log the termination time of the current and the new proxy
                                    FTS3_COMMON_LOGGER_NEWLOG (INFO)
                                            << "Current proxy termination time: "
                                            << cred->terminationTime
                                            << ", new proxy proxy termination time: "
                                            << incomingExpirationTime
                                            << " (the new proxy won't be used)"
                                            << commit;
                                    return;
                                }

                            DBSingleton::instance().getDBObjectInstance()->updateCredential(
                                delegationId,
                                dn,
                                proxy,
                                fqansToString(attrs),
                                incomingExpirationTime
                            );
                        }
                    else
                        {
                            DBSingleton::instance().getDBObjectInstance()->insertCredential(
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
            DBSingleton::instance().getDBObjectInstance()->deleteCredentialCache(delegationId, dn);
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

std::string GSoapDelegationHandler::renewProxyReq(std::string delegationId)
{

    std::string req;
    try
        {
            FTS3_COMMON_LOGGER_NEWLOG (INFO) << "DN: " << dn << " renews proxy certificate" << commit;

            // it is different to gridsite implementation but it is done like that in delegation-java
            // in GliteDelegation.java
            delegationId = handleDelegationId(delegationId);
            if (delegationId.empty()) throw Err_Custom("'handleDelegationId' failed!");

            boost::optional<UserCredentialCache> cache (
                DBSingleton::instance().getDBObjectInstance()->findCredentialCache(delegationId, dn)
            );

            if(cache)
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
                    DBSingleton::instance().getDBObjectInstance()->insertCredentialCache(
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

time_t GSoapDelegationHandler::getTerminationTime(std::string delegationId)
{

    try
        {
            FTS3_COMMON_LOGGER_NEWLOG (INFO) << "DN: " << dn << " gets proxy certificate termination time" << commit;

            std::string delegationId = makeDelegationId();
            if (delegationId.empty()) throw Err_Custom("'getDelegationId' failed!");

            boost::optional<UserCredential> cred (
                DBSingleton::instance().getDBObjectInstance()->findCredential(delegationId, dn)
            );

            if (cred)
                {
                    return cred->terminationTime;
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

void GSoapDelegationHandler::destroy(std::string delegationId)
{

    FTS3_COMMON_LOGGER_NEWLOG (INFO) << "DN: " << dn << " destroys proxy certificate" << commit;

    delegationId = handleDelegationId(delegationId);
    if (delegationId.empty()) throw Err_Custom("'handleDelegationId' failed!");

    try
        {
            DBSingleton::instance().getDBObjectInstance()->deleteCredentialCache(delegationId, dn);
            DBSingleton::instance().getDBObjectInstance()->deleteCredential(delegationId, dn);
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

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
 * ProxyCertificateDelegator.cpp
 *
 *  Created on: Jun 4, 2012
 *      Author: simonm
 */

#include "ProxyCertificateDelegator.h"

#include "exception/cli_exception.h"

#include <openssl/x509_vfy.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/asn1.h>

#include <gridsite.h>

#include <iostream>

namespace fts3
{
namespace cli
{


ProxyCertificateDelegator::ProxyCertificateDelegator(string endpoint, string delegationId, long userRequestedDelegationExpTime):
    delegationId(delegationId),
    endpoint(endpoint),
    userRequestedDelegationExpTime(userRequestedDelegationExpTime)
{

    dctx = glite_delegation_new(endpoint.c_str());
    if (dctx == NULL)
        {
            throw cli_exception("delegation: could not initialise a delegation context");
        }

}

ProxyCertificateDelegator::~ProxyCertificateDelegator()
{

    glite_delegation_free(dctx);
}

long ProxyCertificateDelegator::isCertValid(string filename)
{

    // find user proxy certificate
    FILE* fp;
    if (!filename.empty())
        {
            fp = fopen(filename.c_str(), "r");
        }
    else
        {
            char* user_proxy = GRSTx509FindProxyFileName();
            fp = fopen(user_proxy , "r");
            free (user_proxy);
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

void ProxyCertificateDelegator::delegate()
{

    // the default is to delegate, but not renew
    bool renewDelegation = false, needDelegation = true;

    // get local proxy run time
    time_t localProxyTimeLeft = isCertValid(string());
    MsgPrinter::instance().print_info(
            "Remaining time for the local proxy is",
            "delegation.local_expiration_time",
            (long int)((localProxyTimeLeft) / 3600),
            (long int)((localProxyTimeLeft) % 3600 / 60)
        );

    // checking if there is already a delegated credenial
    time_t expTime;
    int err = glite_delegation_info(dctx, delegationId.c_str(), &expTime);

    if (!err)     // there is already a delegated proxy on the server
        {

            time_t timeLeftOnServer = expTime - time(0);
            MsgPrinter::instance().print_info(
                    "Remaining time for the proxy on the server side is",
                    "delegation.service_expiration_time",
                    (long int)((timeLeftOnServer) / 3600),
                    (long int)((timeLeftOnServer) % 3600 / 60)
                );

            if (timeLeftOnServer > REDELEGATION_TIME_LIMIT)
                {
                    // don;t bother redelegating
                    MsgPrinter::instance().print_info(
                            "delegation.message",
                            "Not bothering to do delegation, since the server already has a delegated credential for this user lasting longer than 4 hours."
                        );
                    needDelegation = false;
                    renewDelegation = false;
                }
            else
                {
                    // think about redelegating
                    if (localProxyTimeLeft > timeLeftOnServer)
                        {
                            // we improve the situation (the new proxy will last longer)
                            MsgPrinter::instance().print_info(
                                    "delegation.message",
                                    "Will redo delegation since the credential on the server has left that 4 hours validity left."
                                );
                            needDelegation = true;
                            renewDelegation = true; // renew rather than put
                        }
                    else
                        {
                            // we cannot improve the proxy on the server
                            MsgPrinter::instance().print_info(
                                    "delegation.message",
                                    "Delegated proxy on server has less than 6 hours left.\nBut the local proxy has less time left than the one on the server, so cannot be used to refresh it!"
                                );
                            needDelegation=false;
                        }
                }
        }
    else
        {
            // no proxy on server: do standard delegation
            MsgPrinter::instance().print_info(
                    "delegation.message",
                    "No proxy found on server. Requesting standard delegation."
                );
            needDelegation = true;
            renewDelegation = false;
        }

    if (needDelegation)
        {

            long requestProxyDelegationTime;

            if (userRequestedDelegationExpTime == 0)
                {
                    requestProxyDelegationTime = (int)localProxyTimeLeft - 60; // 60 seconds off current proxy
                    if (requestProxyDelegationTime > MAXIMUM_TIME_FOR_DELEGATION_REQUEST )
                        {
                            requestProxyDelegationTime = MAXIMUM_TIME_FOR_DELEGATION_REQUEST;
                        }
                    if (requestProxyDelegationTime <= 0)
                        {
                            // using a proxy with less than 1 minute to go
                            throw cli_exception ("Your local proxy has less than 1 minute to run, Please renew it before submitting a job.");
                        }
                }
            else
                {
                    requestProxyDelegationTime = userRequestedDelegationExpTime;
                }

            MsgPrinter::instance().print_info(
                    "Requesting delegated proxy for",
                    "delegation.request_duration",
                    (long int)((requestProxyDelegationTime) / 3600),
                    (long int)((requestProxyDelegationTime) % 3600 / 60)
                );

            err = glite_delegation_delegate(
                      dctx, delegationId.c_str(),
                      (int)(requestProxyDelegationTime/60),
                      renewDelegation
                  );

            if (err == -1)
                {
                    MsgPrinter::instance().print_info(
                            "Credential has been successfully delegated to the service.",
                            "delegation.succeed",
                            false
                        );
                    string errMsg = glite_delegation_get_error(dctx);
//        	printer.delegation_request_error(errMsg);

//            // TODO don't use string value to discover the error (???) do we need retry? yes we do!
                    if (errMsg.find("key values mismatch") != string::npos)
                        {
                            MsgPrinter::instance().print_info("Retrying!", "delegation.retry", true);
                            return delegate();
                        }
                    throw cli_exception(errMsg);
                }

            MsgPrinter::instance().print_info(
                    "Credential has been successfully delegated to the service.",
                    "delegation.succeed",
                    true
                );
        }
}

}
}

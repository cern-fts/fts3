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


#include "ws-ifce/gsoap/gsoap_stubs.h"

#include "GSoapDelegationHandler.h"
#include "../AuthorizationManager.h"

#include "common/Logger.h"

#include <boost/thread/locks.hpp>
#include "../../../../../common/Exceptions.h"

using namespace std;
using namespace fts3::common;
using namespace fts3::ws;


//Serialise delegation request to avoid crashes due to multiple SSL initialisation inside canl
static boost::mutex qm;


int fts3::delegation__getProxyReq(struct soap* soap, std::string _delegationID, struct delegation__getProxyReqResponse &_param_4)
{
    boost::mutex::scoped_lock lock(qm);

    try
        {
            AuthorizationManager::instance().authorize(
                soap,
                AuthorizationManager::DELEG,
                AuthorizationManager::dummy
            );

            GSoapDelegationHandler handler(soap);
            _param_4._getProxyReqReturn = handler.getProxyReq(_delegationID);

        }
    catch (BaseException& ex)
        {

            FTS3_COMMON_LOGGER_NEWLOG (ERR) << "An exception has been caught: " << ex.what() << commit;
            soap_receiver_fault(soap, ex.what(), "DelegationException");
            return SOAP_FAULT;
        }

    return SOAP_OK;
}

int fts3::delegation__getNewProxyReq(struct soap* soap, struct delegation__getNewProxyReqResponse &_param_5)
{
    boost::mutex::scoped_lock lock(qm);

    try
        {
            AuthorizationManager::instance().authorize(
                soap,
                AuthorizationManager::DELEG,
                AuthorizationManager::dummy
            );

            GSoapDelegationHandler handler(soap);
            _param_5.getNewProxyReqReturn = handler.getNewProxyReq();

        }
    catch (BaseException& ex)
        {

            FTS3_COMMON_LOGGER_NEWLOG (ERR) << "An exception has been caught: " << ex.what() << commit;
            soap_receiver_fault(soap, ex.what(), "DelegationException");
            return SOAP_FAULT;
        }

    return SOAP_OK;
}

int fts3::delegation__renewProxyReq(struct soap* soap, std::string _delegationID, struct delegation__renewProxyReqResponse &_param_6)
{

    boost::mutex::scoped_lock lock(qm);

    try
        {
            AuthorizationManager::instance().authorize(
                soap,
                AuthorizationManager::DELEG,
                AuthorizationManager::dummy
            );

            GSoapDelegationHandler handler(soap);
            _param_6._renewProxyReqReturn = handler.renewProxyReq(_delegationID);

        }
    catch(BaseException& ex)
        {

            FTS3_COMMON_LOGGER_NEWLOG (ERR) << "An exception has been caught: " << ex.what() << commit;
            soap_receiver_fault(soap, ex.what(), "DelegationException");
            return SOAP_FAULT;
        }

    return SOAP_OK;
}

int fts3::delegation__putProxy(struct soap* soap, std::string _delegationID, std::string _proxy, struct delegation__putProxyResponse &_param_7)
{
    boost::mutex::scoped_lock lock(qm);

    try
        {
            AuthorizationManager::instance().authorize(
                soap,
                AuthorizationManager::DELEG,
                AuthorizationManager::dummy
            );

            GSoapDelegationHandler handler(soap);
            handler.putProxy(_delegationID, _proxy);

        }
    catch (BaseException& ex)
        {
            FTS3_COMMON_LOGGER_NEWLOG (ERR) << "An exception has been caught: " << ex.what() << commit;
            soap_receiver_fault(soap, ex.what(), "DelegationException");
            return SOAP_FAULT;
        }

    return SOAP_OK;
}

int fts3::delegation__getTerminationTime(struct soap* soap, std::string _delegationID, struct delegation__getTerminationTimeResponse &_param_8)
{

    try
        {
            AuthorizationManager::instance().authorize(
                soap,
                AuthorizationManager::DELEG,
                AuthorizationManager::dummy
            );

            GSoapDelegationHandler handler(soap);
            _param_8._getTerminationTimeReturn = handler.getTerminationTime(_delegationID);

        }
    catch (BaseException& ex)
        {

            FTS3_COMMON_LOGGER_NEWLOG (ERR) << "An exception has been caught: " << ex.what() << commit;
            soap_receiver_fault(soap, ex.what(), "DelegationException");
            return SOAP_FAULT;
        }

    return SOAP_OK;
}

int fts3::delegation__destroy(struct soap* soap, std::string _delegationID, struct delegation__destroyResponse &_param_9)
{
    try
        {
            AuthorizationManager::instance().authorize(
                soap,
                AuthorizationManager::DELEG,
                AuthorizationManager::dummy
            );

            GSoapDelegationHandler handler(soap);
            handler.destroy(_delegationID);

        }
    catch(BaseException& ex)
        {

            FTS3_COMMON_LOGGER_NEWLOG (ERR) << "An exception has been caught: " << ex.what() << commit;
            soap_receiver_fault(soap, ex.what(), "DelegationException");
            return SOAP_FAULT;
        }

    return SOAP_OK;
}

int fts3::delegation__getVersion(struct soap* soap, struct delegation__getVersionResponse &_param_1)
{

//	FTS3_COMMON_LOGGER_NEWLOG (INFO) << "Handling 'delegation__getVersion' request" << commit;
    _param_1.getVersionReturn = "3.7.6-1";

    return SOAP_OK;
}

int fts3::delegation__getInterfaceVersion(struct soap* soap, struct delegation__getInterfaceVersionResponse &_param_2)
{

//	FTS3_COMMON_LOGGER_NEWLOG (INFO) << "Handling 'delegation__getInterfaceVersion' request" << commit;
    _param_2.getInterfaceVersionReturn = "3.7.0";

    return SOAP_OK;
}

int fts3::delegation__getServiceMetadata(struct soap* soap, std::string _key, struct delegation__getServiceMetadataResponse &_param_3)
{

//	FTS3_COMMON_LOGGER_NEWLOG (INFO) << "Handling 'delegation__getServiceMetadata' request" << commit;
    _param_3._getServiceMetadataReturn = "glite-data-fts-service-3.7.6-1";

    return SOAP_OK;
}


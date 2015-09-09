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

#ifndef REQUESTLISTER_H_
#define REQUESTLISTER_H_

#include "ws-ifce/gsoap/gsoap_stubs.h"

#include "db/generic/JobStatus.h"
#include "db/generic/SingleDbInstance.h"

#include "ws/CGsiAdapter.h"
#include "ws/AuthorizationManager.h"

#include <string>
#include <vector>


namespace fts3
{
namespace ws
{

/**
 * RequestLister class takes care of listing submitted requests.
 *
 * Depending on the request (listRequest or listRequest2)
 * different constructors should be used.
 */
class RequestLister
{
public:

    /**
     * Constructor - creates a lister object that should be used
     * by listRequest requests.
     *
     * @param soap - the soap object that is serving the given request
     * @param inGivenStates - the states of interest that should be listed
     */
    RequestLister(::soap* soap, impltns__ArrayOf_USCOREsoapenc_USCOREstring *inGivenStates);

    /**
     * Constructor - creates a lister object that should be used
     * by listRequest2 requests.
     *
     * @param soap - the soap object that is serving the given request
     * @param inGivenStates - the states of interest that should be listed
     * @param dn - user DN
     * @param vo - user VO
     */
    RequestLister(::soap* soap, impltns__ArrayOf_USCOREsoapenc_USCOREstring *inGivenStates, std::string dn, std::string vo, std::string src, std::string dst);

    /**
     * Retrieves job statuses from DB.
     *
     * The impltns__ArrayOf_USCOREtns3_USCOREJobStatus object is created using gSOAP
     * memory-allocation utility, it will be garbage collected! If there is a need
     * to delete it manually gSOAP dedicated functions should be used (in particular
     * 'soap_unlink'!).
     *
     * @return impltns__ArrayOf_USCOREtns3_USCOREJobStatus object containing statuses of interest
     */
    impltns__ArrayOf_USCOREtns3_USCOREJobStatus* list(AuthorizationManager::Level lvl);

    /**
     * Retrieves deletion job statuses from DB.
     *
     * The impltns__ArrayOf_USCOREtns3_USCOREJobStatus object is created using gSOAP
     * memory-allocation utility, it will be garbage collected! If there is a need
     * to delete it manually gSOAP dedicated functions should be used (in particular
     * 'soap_unlink'!).
     *
     * @return impltns__ArrayOf_USCOREtns3_USCOREJobStatus object containing statuses of interest
     */
    impltns__ArrayOf_USCOREtns3_USCOREJobStatus* listDm(AuthorizationManager::Level lvl);

    /**
     * Destructor
     */
    virtual ~RequestLister();

private:

    // pointer to GenericDbIfce member that queries jobs
    typedef void (GenericDbIfce::* query_t)(const std::vector<std::string>&,
        const std::string&, const std::string&, const std::string&,
        const std::string&, const std::string&, std::vector<JobStatus>&);

    /**
     * implements jobs listing
     *
     * @param lvl : authorization level
     * @param query : the DB API
     *
     * @return impltns__ArrayOf_USCOREtns3_USCOREJobStatus object containing statuses of interest
     */
    impltns__ArrayOf_USCOREtns3_USCOREJobStatus* list_impl(AuthorizationManager::Level lvl, query_t list);

    /**
     * Default constructor.
     *
     * Private, should not be used.
     */
    RequestLister();

    /**
     * Check weather the states given in impltns__ArrayOf_USCOREsoapenc_USCOREstring
     * are correct, if not a impltns__InvalidArgumentException is thrown
     *
     * @param inGivenStates - the states of interest that should be listed
     */
    void checkGivenStates(impltns__ArrayOf_USCOREsoapenc_USCOREstring *inGivenStates);

    /// the job statuses retrived from the DB
    std::vector<JobStatus> jobs;

    /// the soap object that is serving the given request
    ::soap* soap;

    /// gSOAP cgsi context
    CGsiAdapter cgsi;

    /// DN used for listing jobs
    std::string dn;

    /// VO used for listing jobs
    std::string vo;

    /// source SE
    std::string src;

    /// destination SE
    std::string dst;

    /// the states of interest
    std::vector<std::string> inGivenStates;

    ///
    GenericDbIfce & db;
};

}
}

#endif /* REQUESTLISTER_H_ */

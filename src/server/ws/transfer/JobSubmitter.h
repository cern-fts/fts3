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
 *	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or impltnsied.
 *	See the License for the specific language governing permissions and
 *	limitations under the License.
 *
 * JobSubmitter.h
 *
 *  Created on: Mar 7, 2012
 *      Author: Michal Simon
 */

#ifndef JOBSUBMITTER_H_
#define JOBSUBMITTER_H_

#include "ws-ifce/gsoap/gsoap_stubs.h"
#include "db/generic/GenericDbIfce.h"
#include "common/JobParameterHandler.h"

#include <string>
#include <map>
#include <vector>
#include <list>
#include <utility>

#include <boost/regex.hpp>
#include <boost/tuple/tuple.hpp>

namespace fts3
{
namespace ws
{

using namespace std;
using namespace fts3::common;
using namespace boost;

/**
 * The JobSubmitter class takes care of submitting transfers
 *
 * Depending on the request (submitTransfer, submitTransfer2, submitTransfer3)
 * different constructors should be used
 *
 */
class JobSubmitter
{

    enum
    {
        SHARE,
        CONTENT
    };

    enum
    {
        SOURCE = 0,
        DESTINATION,
        VO
    };

    struct TimeoutHandler
    {
        TimeoutHandler(map<string, int>& timeouts) : timeouts(timeouts) {}

        template<typename T>
        void operator ()(T& t)
        {
            string& src = t.source_se;
            string& dst = t.dest_se;
            // if theres's nothing to do continue
            if (timeouts.find(src) == timeouts.end() && timeouts.find(dst) == timeouts.end()) return;
            // if src has no timeout use destination and continue
            if (timeouts.find(src) == timeouts.end())
                {
                    t.wait_timeout = timeouts[dst];
                    return;
                }
            // if dst has no time use source
            if (timeouts.find(dst) == timeouts.end())
                {
                    t.wait_timeout = timeouts[src];
                    return;
                }
            // if both dst and src have timeout pick the lower value
            t.wait_timeout = timeouts[src] < timeouts[dst] ? timeouts[src] : timeouts[dst];
        }

        map<string, int> timeouts;
    };

public:
    /**
     * Constructor - creates a submitter object that should be used
     * by submitTransfer and submitTransfer2 requests
     *
     * @param soap - the soap object that is serving the given request
     * @param job - the job that has to be submitted
     * @param delegation - should be true if delegation is being used
     */
    JobSubmitter(soap* soap, tns3__TransferJob *job, bool delegation);

    /**
     * Constructor - creates a submitter object that should be used
     * by submitTransfer3 requests
     *
     * @param soap - the soap object that is serving the given request
     * @param job - the job that has to be submitted
     */
    JobSubmitter(soap* soap, tns3__TransferJob2 *job);

    /**
     * Constructor - creates a submitter object that should be used
     * by submitTransfer4 requests
     *
     * @param soap - the soap object that is serving the given request
     * @param job - the job that has to be submitted
     */
    JobSubmitter(soap* ctx, tns3__TransferJob3 *job);

    /**
     * Destructor
     */
    virtual ~JobSubmitter();

    /**
     * submits the job
     */
    string submit();

private:

    /// DB instance
    GenericDbIfce* db;

    /**
     * Default constructor.
     *
     * Private, should not be used.
     */
    JobSubmitter() {};

    /// job ID
    string id;
    /// user DN
    string dn;
    /// user VO
    string vo;
    /// delegation ID
    string delegationId;
    /// user password
    string cred;
    /// copy lifetime pin
    int copyPinLifeTime;

    /// maps job parameter values to their names
    JobParameterHandler params;

    /**
     * the job elements that have to be submitted (each job is a tuple of source,
     * destination, and optionally checksum)
     */
    list<job_element_tupple> jobs;

    /**
     * The common initialization for both parameterized constructors
     *
     * @param jobParams - job parameters
     */
    void init(tns3__TransferParams *jobParams);

    /**
     * Extracts the activity name from file metadata
     * in case if there is no activity it returns 'default'
     */
    string getActivity(string metadata);

    /**
     * Checks:
     * - if the SE has been blacklisted (if yes an exception is thrown)
     * - if the SE is in BDII and the state is either 'production' or 'online'
     *   (if it is in BDII but the state is wrong an exception is thrown)
     * - if the SE is in BDII and the submitted VO is on the VOsAllowed list
     *   (if it is in BDII but the VO is not on the list an exception is thrown)
     * - if the SE is in the OSG and if is's active and not disabled
     *   (it it is in BDII but the conditions are not met an exception is thrown)
     */
    void checkSe(string ses, string vo);

    /**
     * Checks whether the right protocol has been used
     *
     * @file - source or destination file
     * @return true if right protol has been used
     */
    void checkProtocol(string file, bool source);

    void pairSourceAndDestination(vector<string>& sources, vector<string>& destinations, string& selectionStrategy, list< pair<string, string> >& ret);

    inline void addSe(string& se);

    inline string getSesStr();

    ///
    static const regex fileUrlRegex;

    static const string false_str;


    static const string srm_protocol;

    bool srm_source;

    /**
     *
     */
    string fileUrlToSeName(string url, bool source = false);

    string sourceSe;
    string destinationSe;

    set<string> uniqueSes;
    string uniqueSesStr;

};

}
}

#endif /* JOBSUBMITTER_H_ */

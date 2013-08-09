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
 * GSoapContextAdapter.h
 *
 *  Created on: May 30, 2012
 *      Author: Michał Simon
 */

#ifndef GSOAPCONTEXADAPTER_H_
#define GSOAPCONTEXADAPTER_H_

#include "TransferTypes.h"
#include "ws-ifce/gsoap/gsoap_stubs.h"

#include <vector>
#include <map>

using namespace std;

namespace fts3
{
namespace cli
{

/**
 * The adapter class for the GSoap context
 *
 * This one is used instead of GSoap generated proxy classes because of GSoap bug ID: 3511337
 * (see example http://sourceforge.net/tracker/?func=detail&atid=468021&aid=3511337&group_id=52781)
 *
 * Provides all the functionalities of transfer and configuration web service
 */
class GSoapContextAdapter
{

public:

    /**
     * Constructor
     *
     * Creates and initializes GSoap context
     */
    GSoapContextAdapter(string endpoint);

    /**
     * Destructor
     *
     * Deallocates GSoap context
     */
    virtual ~GSoapContextAdapter();

    /**
     * Type cast operator.
     *
     * @return pointer do gsoap context (soap*)
     */
    operator soap*()
    {
        return ctx;
    }


    /**
     * Initializes CGSI plugin for the soap object, respectively to the protocol that is used (https, httpg,..).
     * 		Moreover, sets the namespaces.
     * Initializes the object with service version, metadata, schema version and interface version.
     *
     * @return true if all information were received
     * @see SrvManager::initSoap(soap*, string)
     */
    void init();

    /**
     * Handles soap fault. Calls soap_stream_fault, then throws a string exception with given message
     *
     * @param msg exception message
     */
    void handleSoapFault(string msg);


    /**
     * Remote call that will be transferSubmit2 or transferSubmit3
     *
     * @param elements - job elements to be executed
     * @param parameters - parameters for the job that is being submitted
     * @param checksum - flag indicating whether the checksum should be used
     * 	(if false transferSubmit2 is used, otherwise transferSubmit3 is used)
     *
     * @return the job ID
     */
    string transferSubmit (vector<File> files, map<string, string> parameters);

    /**
     * Remote call to transferSubmit
     *
     * @param elements - job elements to be executed
     * @param parameters - parameters for the job that is being submitted
     * @param password - user credential that is being used instead of certificate delegation
     *
     * @return the job ID
     */
    string transferSubmit (vector<JobElement> elements, map<string, string> parameters, string password);

    /**
     * Remote call to getTransferJobStatus
     *
     * @param jobId   the job id
     * @param archive if true, the archive will be queried
     *
     * @return an object holding the job status
     */
    JobStatus getTransferJobStatus (string jobId, bool archive);

    /** TODO
     * Remote call to getRoles
     *
     * @param resp server response (roles)
     */
    void getRoles (impltns__getRolesResponse& resp);

    /** TODO
     * Remote call to getRolesOf
     *
     * @param resp server response (roles)
     */
    void getRolesOf (string dn, impltns__getRolesOfResponse& resp);

    /**
     * Remote call to cancel
     *
     * @param jobIds list of job IDs
     */
    void cancel(vector<string> jobIds);

    /**
     * Remote call to listRequests
     * Internally is listRequests2
     *
     * @param dn user dn
     * @param vo vo name
     * @param array statuses of interest
     * @param resp server response
     */
    vector<JobStatus> listRequests (vector<string> statuses, string dn, string vo);

    /** TODO
     * Remote call to listVOManagers
     *
     * @param vo vo name
     * @param resp server response
     */
    void listVoManagers (string vo, impltns__listVOManagersResponse& resp);

    /**
     * Remote call to getTransferJobSummary
     * Internally it is getTransferJobSummary3
     *
     * @param jobId   id of the job
     * @param archive if true, the archive will be queried
     *
     * @return an object containing job summary
     */
    JobSummary getTransferJobSummary (string jobId, bool archive);

    /**
     * Remote call to getFileStatus
     *
     * @param jobId   id of the job
     * @param archive if true, the archive will be queried
     * @param offset  query starting from this offset (i.e. files 100 in advance)
     * @param limit   query a limited number of files (i.e. only 50 results)
     * @param resp server response
     * @return The number of files returned
     */
    int getFileStatus (string jobId, bool archive, int offset, int limit,
                       impltns__getFileStatusResponse& resp);

    /**
     * Remote call to setConfiguration
     *
     * @param config th configuration to be set
     * @param resp server response
     */
    void setConfiguration (config__Configuration *config, implcfg__setConfigurationResponse& resp);

    /**
     * Remote call to getConfiguration
     *
     * @param vo - vo name that will be used to filter the response
     * @param name - SE or SE group name that will be used to filter the response
     * @param resp - server response
     */
    void getConfiguration (string src, string dest, string all, string name, implcfg__getConfigurationResponse& resp);

    /**
     * Remote call to delConfiguration
     *
     * @param cfg - the configuration that will be deleted
     * @param resp - server response
     */
    void delConfiguration(config__Configuration *config, implcfg__delConfigurationResponse &resp);

    /**
     * Remote call to setBringOnline
     *
     * @param pairs - se name - max number staging files pairs
     */
    void setBringOnline(map<string, int>& pairs);

    /**
     * Splits the given string, and sets:
     * 		- major number
     * 		- minor number
     * 		- patch number
     *
     * @param interface - interface version of FTS3 service
     */
    void setInterfaceVersion(string interface);

    /**
     * TODO
     */
    void debugSet(string source, string destination, bool debug);

    /**
     * TODO
     */
    void blacklistDn(string subject, string status, int timeout, bool mode);

    void blacklistSe(string name, string vo, string status, int timeout, bool mode);

    /**
     * TODO
     */
    void doDrain(bool drain);

    /**
     * TODO
     */
    void prioritySet(string jobId, int priority);

    /**
     * TODO
     */
    void retrySet(int retry);

    /**
     *
     */
    void optimizerModeSet(int mode);

    /**
     * TODO
     */
    void queueTimeoutSet(unsigned timeout);

    /**
     * TODO
     */
    void getLog(string& logname, string jobId);

    ///@{
    /**
     * A group of methods returning details about interface version of the FTS3 service
     */
    string getEndpoint()
    {
        return endpoint;
    }

    string getInterface()
    {
        return interface;
    }

    string getVersion()
    {
        return version;
    }

    string getSchema()
    {
        return schema;
    }

    string getMetadata()
    {
        return metadata;
    }
    ///@}

private:

    ///
    string endpoint;

    ///
    soap* ctx;

    ///@{
    /**
     * Interface Version components
     */
    long major;
    long minor;
    long patch;
    ///@}

    ///@{
    /**
     * general informations about the FTS3 service
     */
    string interface;
    string version;
    string schema;
    string metadata;
    ///@}

};

}
}

#endif /* GSOAPCONTEXADAPTER_H_ */

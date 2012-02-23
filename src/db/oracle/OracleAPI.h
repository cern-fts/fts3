/********************************************//**
 * Copyright @ Members of the EMI Collaboration, 2010.
 * See www.eu-emi.eu for details on the copyright holders.
 * 
 * Licensed under the Apache License, Version 2.0 (the "License"); 
 * you may not use this file except in compliance with the License. 
 * You may obtain a copy of the License at 
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0 
 * 
 * Unless required by applicable law or agreed to in writing, software 
 * distributed under the License is distributed on an "AS IS" BASIS, 
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
 * See the License for the specific language governing permissions and 
 * limitations under the License.
 ***********************************************/

/**
 * @file OracleAPI.h
 * @brief interface to access oracle backend
 * @author Michail Salichos
 * @date 09/02/2012
 * 
 **/

#pragma once

#include "GenericDbIfce.h"
#include "OracleConnection.h"
#include <boost/scoped_ptr.hpp>

class OracleAPI : public GenericDbIfce {
public:
    OracleAPI();
    virtual ~OracleAPI();
    
    virtual void init(std::string username, std::string password, std::string connectString);    

    virtual void submitPhysical(const std::string & jobId, std::map<std::string, std::string> src_dest_pair, const std::string & paramFTP,
                                 const std::string & DN, const std::string & cred, const std::string & voName, const std::string & myProxyServer,
                                 const std::string & delegationID, const std::string & spaceToken, const std::string & overwrite, 
                                 const std::string & sourceSpaceToken, const std::string & sourceSpaceTokenDescription, const std::string & lanConnection, int copyPinLifeTime,
                                 const std::string & failNearLine, const std::string & checksum, const std::string & checksumMethod);

    virtual JobStatus* getTransferJobStatus(std::string requestID);

    virtual std::vector<std::string> getSiteGroupNames();

    virtual std::vector<std::string> getSiteGroupMembers(std::string GroupName);

    virtual void removeGroupMember(std::string groupName, std::string siteName);

    virtual void addGroupMember(std::string groupName, std::string siteName);

    /*virtual void setVOLimit(std::string channelUpperName, std::string voName, int limit);

    virtual std::vector<JobStatus> listRequests(std::vector<std::string> inGivenStates, std::string channelName, std::string restrictToClientDN, std::string forDN, std::string VOname);

    virtual std::vector<FileTransferStatus> getFileStatus(std::string requestID, int offset, int limit);



    virtual TransferJobSummary* getTransferJobSummary(std::string requestID);

    virtual void cancel(std::vector<std::string> requestIDs);

    virtual void addChannel(std::string channelName, std::string sourceSite, std::string destSite, std::string contact, int numberOfStreams,
            int numberOfFiles, int bandwidth, int nominalThroughput, std::string state);

    virtual void setJobPriority(std::string requestID, int priority);

    virtual SePair* getSEPairName(std::string sePairName);

    virtual void dropChannel(std::string name);

    virtual void setState(std::string channelName, std::string state, std::string message);

    virtual std::vector<std::string> listChannels();

    virtual void setNumberOfStreams(std::string channelName, int numberOfStreams, std::string message);


    virtual void setNumberOfFiles(std::string channelName, int numberOfFiles, std::string message);


    virtual void setBandwidth(std::string channelName, int utilisation, std::string message);


    virtual void setContact(std::string channelName, std::string contact, std::string message);


    virtual void setNominalThroughput(std::string channelName, int nominalThroughput, std::string message);


    virtual void changeStateForHeldJob(std::string jobID, std::string state);


    virtual void changeStateForHeldJobs(std::string channelName, std::string state);


    virtual void addChannelManager(std::string channelName, std::string principal);


    virtual void removeChannelManager(std::string channelName, std::string principal);


    virtual std::vector<std::string> listChannelManagers(std::string channelName);

    virtual std::map<std::string, std::string> getChannelManager(std::string channelName, std::vector<std::string> principals);


    virtual void addVOManager(std::string VOName, std::string principal);


    virtual void removeVOManager(std::string VOName, std::string principal);


    virtual std::vector<std::string> listVOManagers(std::string VOName);

    virtual std::map<std::string, std::string> getVOManager(std::string VOName, std::vector<std::string> principals);


    virtual bool isRequestManager(std::string requestID, std::string clientDN, std::vector<std::string> principals, bool includeOwner);


    virtual void removeVOShare(std::string channelName, std::string VOName);


    virtual void setVOShare(std::string channelName, std::string VOName, int share);


    virtual bool isAgentAvailable(std::string name, std::string type);


    virtual std::string getSchemaVersion();

    virtual void setTcpBufferSize(std::string channelName, std::string bufferSize, std::string message);


    virtual void setTargetDirCheck(std::string channelName, int targetDirCheck, std::string message);


    virtual void setUrlCopyFirstTxmarkTo(std::string channelName, int urlCopyFirstTxmarkTo, std::string message);


    virtual void setChannelType(std::string channelName, std::string channelType, std::string message);


    virtual void setBlockSize(std::string channelName, std::string blockSize, std::string message);


    virtual void setHttpTimeout(std::string channelName, int httpTimeout, std::string message);


    virtual void setTransferLogLevel(std::string channelName, std::string transferLogLevel, std::string message);


    virtual void setPreparingFilesRatio(std::string channelName, double preparingFilesRatio, std::string message);


    virtual void setUrlCopyPutTimeout(std::string channelName, int urlCopyPutTimeout, std::string message);


    virtual void setUrlCopyPutDoneTimeout(std::string channelName, int urlCopyPutDoneTimeout, std::string message);


    virtual void setUrlCopyGetTimeout(std::string channelName, int urlCopyGetTimeout, std::string message);


    virtual void setUrlCopyGetDoneTimeout(std::string channelName, int urlCopyGetDoneTimeout, std::string message);


    virtual void setUrlCopyTransferTimeout(std::string channelName, int urlCopyTransferTimeout, std::string message);


    virtual void setUrlCopyTransferMarkersTimeout(std::string channelName, int urlCopyTransferMarkersTimeout, std::string message);


    virtual void setUrlCopyNoProgressTimeout(std::string channelName, int urlCopyNoProgressTimeout, std::string message);


    virtual void setUrlCopyTransferTimeoutPerMB(std::string channelName, double urlCopyTransferTimeoutPerMB, std::string message);


    virtual void setSrmCopyDirection(std::string channelName, std::string srmCopyDirection, std::string message);


    virtual void setSrmCopyTimeout(std::string channelName, int srmCopyTimeout, std::string message);


    virtual void setSrmCopyRefreshTimeout(std::string channelName, int srmCopyRefreshTimeout, std::string message);

    virtual void removeVOLimit(std::string channelUpperName, std::string voName);

    virtual void addSe( std::string ENDPOINT, std::string SE_TYPE, std::string SITE, std::string NAME, std::string STATE, std::string VERSION, std::string HOST,
    std::string  SE_TRANSFER_TYPE, std::string   SE_TRANSFER_PROTOCOL, std::string   SE_CONTROL_PROTOCOL, std::string GOCDB_ID);

    virtual Se* getSeInfo(std::string ENDPOINT, std::string SE_TYPE, std::string SITE, std::string NAME, std::string STATE, std::string VERSION, std::string HOST,
    std::string  SE_TRANSFER_TYPE, std::string   SE_TRANSFER_PROTOCOL, std::string   SE_CONTROL_PROTOCOL, std::string GOCDB_ID);

    virtual void updateSe( std::string ENDPOINT, std::string SE_TYPE, std::string SITE, std::string NAME, std::string STATE, std::string VERSION, std::string HOST,
    std::string  SE_TRANSFER_TYPE, std::string   SE_TRANSFER_PROTOCOL, std::string   SE_CONTROL_PROTOCOL, std::string GOCDB_ID);

    virtual void deleteSe( std::string ENDPOINT, std::string SITE, std::string NAME);
    */


private:
	OracleConnection *conn;
};

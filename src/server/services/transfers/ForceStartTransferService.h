//
// Created by Tom Hepworth on 29/08/2022.
//

#pragma once

#include "../BaseService.h"
#include "../heartbeat/HeartBeat.h"

namespace fts3 {
namespace server {

class ForceStartTransferService: public BaseService
{
public:
    ForceStartTransferService(HeartBeat *beat);
    virtual void runService();

protected:
    std::string ftsHostName;
    std::string infosys;
    bool monitoringMessages;
    int execPoolSize;
    std::string logDir;
    std::string msgDir;
    boost::posix_time::time_duration pollInterval;

    HeartBeat *beat;
    void forceRunJobs();
};

} // end namespace server
} // end namespace fts3


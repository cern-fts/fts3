/**
 *  Copyright (c) Members of the EGEE Collaboration. 2004.
 *  See http://www.eu-egee.org/partners/ for details on the copyright
 *  holders.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 * This following code is to used for generating information retrieved from the database
 * regarding the load of the channels, ratio of active/max, etc.
 * It is executed hourly
 */

#include <string>
#include <sstream>
#include <iomanip>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <libgen.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <iostream>
#include <vector>
#include <db/generic/SingleDbInstance.h>
#include "utility_routines.h"
#include "Logger.h"
#include <memory>
#include <exception>
#include "config/serverconfig.h"
#include "common/logger.h"
#include "common/error.h"

using namespace std;
using namespace FTS3_COMMON_NAMESPACE;
using namespace FTS3_CONFIG_NAMESPACE;


class MsgProducer
{
private:
    vector <std::string> credentials;
    string sql;
    string sql2;
    MonitoringDbIfce* monitoringDb;
    double active;
    double max;
    double ratio;

public:

    MsgProducer()
    {
        this->monitoringDb = NULL;
        this->active = 0.0;
        this->max = 0.0;
        this->ratio = 0.0;
    }

    virtual ~MsgProducer()
    {
        cleanup();
    }

    virtual void run()
    {
        static int links_found = 0;
        char hostname[1024] = {0};
        gethostname(hostname, 1024);
        string host = theServerConfig().get<std::string>("Alias");

        try
            {
                bool fileExists = get_mon_cfg_file();

                if ((fileExists == false) || (false == getACTIVE()) || (false == getResolveAlias()))
                    {
                        logger::writeLog("Check the msg config file to see if the connection is active and the FQDN/alias of this machine", true);
                        exit(0);
                    }

                std::string dbUserName = theServerConfig().get<std::string>("DbUserName");
                std::string dbPassword = theServerConfig().get<std::string>("DbPassword");
                std::string dbConnectString = theServerConfig().get<std::string>("DbConnectString");

                try
                    {
                        this->monitoringDb = db::DBSingleton::instance().getMonitoringDBInstance();
                        this->monitoringDb->init(dbUserName, dbPassword, dbConnectString, 2);
                    }
                catch (Err& exc)
                    {
                        logger::writeLog(std::string("Cannot connect to the database server: ") + exc.what());
                        exit(1);
                    }

                // Create a message
                string text = "{";
                // fts instance identifier
                text += "\"fts_id\":\"";
                text += "" + host + "";
                //text += ""; //null values
                text += "\"";
                // milliseconds since epoc time stamp of the report
                text += ",\"time\":\"";
                text += _getTimestamp();
                text += "\",";

                std::vector<std::string> distinctVOs;
                monitoringDb->getVONames(distinctVOs);

                text += "\"vos\": [";

                for (size_t voIndex = 0; voIndex < distinctVOs.size(); ++voIndex)
                    {
                        std::string& vo = distinctVOs[voIndex];

                        // Create an array of the channels
                        text += "{\"voname\":\"" + vo + "\",\"channels\":[";

                        // for every pair source_ dest_ host collect information
                        std::vector<SourceAndDestSE> pairs;
                        monitoringDb->getSourceAndDestSEForVO(vo, pairs);
                        for (size_t pairIndex = 0; pairIndex < pairs.size(); ++pairIndex)
                            {
                                SourceAndDestSE& pair = pairs[pairIndex];

                                ++links_found;
                                text += "{";
                                text += "\"channel_name\":\"";
                                text += pair.sourceStorageElement + "__" + pair.destinationStorageElement;
                                text += "\",";
                                text += "\"channel_type\":\"\",";

                                text += "\"links\": [{";

                                text += "\"source_host\":\"";
                                text += pair.sourceStorageElement;
                                text += "\"";
                                text += ",\"dest_host\":\"";
                                text += pair.destinationStorageElement;
                                text += "\"";

                                std::vector<std::string> states;
                                states.push_back("ACTIVE");
                                unsigned active = monitoringDb->numberOfTransfersInState(vo, pair, states);
                                states.clear();
                                states.push_back("SUBMITTED");
                                states.push_back("READY");
                                unsigned ready  = monitoringDb->numberOfTransfersInState(vo, pair, states);

                                text += ",\"active\":\"";
                                text += _to_string<unsigned>(active, std::dec);
                                text += "\"";
                                text += ",\"ratio\":\"";
                                text += _to_string<double>(ratio, std::dec);
                                text += "%\"";
                                text += ",\"ready\":\"";
                                text += _to_string<unsigned>(ready, std::dec);
                                text += "\"";
                                text += "}]},";
                            }
                        if (links_found > 0)
                            text.resize(text.size() - 1);

                        text += "]},";

                        links_found = 0;
                        //text.resize(text.size() - 1);
                    }

                if(!distinctVOs.empty())
                    text.resize(text.size() - 1);
                text += "]}";
                text += 4; /*add EOT ctrl character*/

                //logger::writeLog("PE " + text);
            }
        catch (const std::exception& e)
            {
                std::string errorMessage = "Unrecovereable error occured: " + std::string(e.what());
                logger::writeLog(errorMessage, true);
            }
        catch (...)
            {
                logger::writeLog("Unexpected exception", true);

            }
    }

private:

    void cleanup()
    {

        // Close DB
        try
            {
                db::DBSingleton::tearDown();
            }
        catch (std::exception& e)
            {
                logger::writeLog(e.what(), true);
            }
        catch (...)
            {
                logger::writeLog("Unknown error occured", true);
            }
    }
};

int main(int argc, char** argv)
{

    try
        {
            FTS3_CONFIG_NAMESPACE::theServerConfig().read(argc, argv);
            //MsgProducer producer;
            //producer.run();
        }
    catch (const std::exception& e)
        {
            std::cerr << "Exception caught: " << e.what() << std::endl;
            return -1;
        }
    catch (...)
        {
            std::cerr << "Unexpected exception" << std::endl;
            return -1;
        }

    return 0;

}

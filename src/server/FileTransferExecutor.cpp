/*
 * FileTransferExecutor.cpp
 *
 *  Created on: Aug 9, 2013
 *      Author: simonm
 */

#include <stdlib.h>
#include <boost/scoped_ptr.hpp>

#include "FileTransferExecutor.h"

#include "common/parse_url.h"
#include "common/OptimizerSample.h"
#include "common/logger.h"
#include "common/name_to_uid.h"
#include "common/producer_consumer_common.h"
#include "common/queue_updater.h"

#include "ConfigurationAssigner.h"
#include "FileTransferScheduler.h"
#include "ProtocolResolver.h"
#include "process.h"

#include "ws/SingleTrStateInstance.h"

#include "cred/cred-utility.h"

extern bool stopThreads;




namespace fts3
{

namespace server
{

const string FileTransferExecutor::cmd = "fts_url_copy";


FileTransferExecutor::FileTransferExecutor(TransferFileHandler& tfh, bool optimize, bool monitoringMsg, string infosys, string ftsHostName) :
    tfh(tfh),
    optimize(optimize),
    monitoringMsg(monitoringMsg),
    infosys(infosys),
    ftsHostName(ftsHostName),
    db(DBSingleton::instance().getDBObjectInstance()),
    t(bind(&FileTransferExecutor::execute, this)),
    active(true),
    scheduled(0)

{
    char hostname[1024] = {0};
    hostname[1023] = '\0';
    gethostname(hostname, 1023);
    ftsHostname = std::string(hostname);
}

FileTransferExecutor::~FileTransferExecutor()
{
    stop();
}

string FileTransferExecutor::prepareMetadataString(std::string text)
{
    text = boost::replace_all_copy(text, " ", "?");
    text = boost::replace_all_copy(text, "\"", "\\\"");
    return text;
}

void FileTransferExecutor::execute()
{
    while (active && queue.hasData() && !stopThreads)
        {
            try
                {
                    // get the file from the queue
                    scoped_ptr<TransferFiles> temp (queue.get());

                    // make sure it's not a NULL
                    if (!temp.get()) continue;

                    string source_hostname = temp->SOURCE_SE;
                    string destin_hostname = temp->DEST_SE;

                    // if the pair was already checked and not scheduled skip it
                    if (notScheduled.count(make_pair(source_hostname, destin_hostname))) continue;

                    /*check if manual config exist for this pair and vo*/

                    vector< boost::shared_ptr<ShareConfig> > cfgs;

                    ConfigurationAssigner cfgAssigner(temp.get());
                    cfgAssigner.assign(cfgs);

                    SeProtocolConfig protocol;

                    int BufSize = 0;
                    int StreamsperFile = 0;
                    int Timeout = 0;

                    std::stringstream internalParams;

                    bool manualConfigExists = false;

                    if (optimize)
                        {
                            OptimizerSample* opt_config = new OptimizerSample();
                            db->initOptimizer(source_hostname, destin_hostname, 0);
                            db->fetchOptimizationConfig2(opt_config, source_hostname, destin_hostname);
                            BufSize = opt_config->getBufSize();
                            StreamsperFile = opt_config->getStreamsperFile();
                            Timeout = opt_config->getTimeout();
                            delete opt_config;
                            opt_config = NULL;
                        }

                    FileTransferScheduler scheduler(
                        temp.get(),
                        cfgs,
                        tfh.getDestinations(source_hostname),
                        tfh.getSources(destin_hostname),
                        tfh.getDestinationsVos(source_hostname),
                        tfh.getSourcesVos(destin_hostname)

                    );

                    if (scheduler.schedule(optimize))   /*SET TO READY STATE WHEN TRUE*/
                        {
                            ++scheduled;

                            SingleTrStateInstance::instance().sendStateMessage(temp->JOB_ID, temp->FILE_ID);
                            bool isAutoTuned = false;

                            if (optimize && cfgs.empty())
                                {
                                    DBSingleton::instance().getDBObjectInstance()->setAllowed(temp->JOB_ID, temp->FILE_ID, source_hostname, destin_hostname, StreamsperFile, Timeout, BufSize);
                                }
                            else
                                {
                                    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Check link config for: " << source_hostname << " -> " << destin_hostname << commit;
                                    ProtocolResolver resolver(temp.get(), cfgs);
                                    bool protocolExists = resolver.resolve();
                                    if (protocolExists)
                                        {
                                            manualConfigExists = true;
                                            protocol.NOSTREAMS = resolver.getNoStreams();
                                            protocol.NO_TX_ACTIVITY_TO = resolver.getNoTxActiveTo();
                                            protocol.TCP_BUFFER_SIZE = resolver.getTcpBufferSize();
                                            protocol.URLCOPY_TX_TO = resolver.getUrlCopyTxTo();

                                            if (protocol.NOSTREAMS >= 0)
                                                internalParams << "nostreams:" << protocol.NOSTREAMS;
                                            if (protocol.URLCOPY_TX_TO >= 0)
                                                internalParams << ",timeout:" << protocol.URLCOPY_TX_TO;
                                            if (protocol.TCP_BUFFER_SIZE >= 0)
                                                internalParams << ",buffersize:" << protocol.TCP_BUFFER_SIZE;
                                        }
                                    else
                                        {
                                            internalParams << "nostreams:" << DEFAULT_NOSTREAMS << ",timeout:" << DEFAULT_TIMEOUT << ",buffersize:" << DEFAULT_BUFFSIZE;
                                        }

                                    if (resolver.isAuto())
                                        {
                                            isAutoTuned = true;
                                            DBSingleton::instance().getDBObjectInstance()->setAllowed(
                                                temp->JOB_ID,
                                                temp->FILE_ID,
                                                source_hostname,
                                                destin_hostname,
                                                resolver.getNoStreams(),
                                                resolver.getNoTxActiveTo(),
                                                resolver.getTcpBufferSize()
                                            );
                                        }
                                    else
                                        {
                                            DBSingleton::instance().getDBObjectInstance()->setAllowedNoOptimize(
                                                temp->JOB_ID,
                                                temp->FILE_ID,
                                                internalParams.str()
                                            );
                                        }
                                }

                            string proxy_file = get_proxy_cert(
                                                    temp->DN, // user_dn
                                                    temp->CRED_ID, // user_cred
                                                    temp->VO_NAME, // vo_name
                                                    "",
                                                    "", // assoc_service
                                                    "", // assoc_service_type
                                                    false,
                                                    ""
                                                );

			    //temporarly disabled BDII access for getting the site name, pls do not remove the following 2 lines
                            string sourceSiteName = ""; //siteResolver.getSiteName(temp->SOURCE_SURL);
                            string destSiteName = ""; //siteResolver.getSiteName(temp->DEST_SURL);

                            bool debug = DBSingleton::instance().getDBObjectInstance()->getDebugMode(source_hostname, destin_hostname);

                            string params;

                            if (debug == true)
                                {
                                    params.append(" -F ");
                                }

                            if (manualConfigExists)
                                {
                                    params.append(" -N ");
                                }

                            if (isAutoTuned)
                                {
                                    params.append(" -O ");
                                }

                            if (monitoringMsg)
                                {
                                    params.append(" -P ");
                                }

                            if (proxy_file.length() > 0)
                                {
                                    params.append(" -proxy ");
                                    params.append(proxy_file);
                                }

                            if (std::string(temp->CHECKSUM).length() > 0)   //checksum
                                {
                                    params.append(" -z ");
                                    params.append(temp->CHECKSUM);
                                }
                            if (std::string(temp->CHECKSUM_METHOD).length() > 0)   //checksum
                                {
                                    params.append(" -A ");
                                    params.append(temp->CHECKSUM_METHOD);
                                }
                            params.append(" -b ");
                            params.append(temp->SOURCE_SURL);
                            params.append(" -c ");
                            params.append(temp->DEST_SURL);
                            params.append(" -a ");
                            params.append(temp->JOB_ID);
                            params.append(" -B ");
                            params.append(lexical_cast<string >(temp->FILE_ID));
                            params.append(" -C ");
                            params.append(temp->VO_NAME);
                            if (sourceSiteName.length() > 0)
                                {
                                    params.append(" -D ");
                                    params.append(sourceSiteName);
                                }
                            if (destSiteName.length() > 0)
                                {
                                    params.append(" -E ");
                                    params.append(destSiteName);
                                }
                            if (std::string(temp->OVERWRITE).length() > 0)
                                {
                                    params.append(" -d ");
                                }
                            if (optimize && manualConfigExists == false)
                                {
                                    params.append(" -e ");
                                    params.append(lexical_cast<string >(StreamsperFile));
                                }
                            else
                                {
                                    if (protocol.NOSTREAMS >= 0)
                                        {
                                            params.append(" -e ");
                                            params.append(lexical_cast<string >(protocol.NOSTREAMS));
                                        }
                                    else
                                        {
                                            params.append(" -e ");
                                            params.append(lexical_cast<string >(DEFAULT_NOSTREAMS));
                                        }
                                }

                            if (optimize && manualConfigExists == false)
                                {
                                    params.append(" -f ");
                                    params.append(lexical_cast<string >(BufSize));
                                }
                            else
                                {
                                    if (protocol.TCP_BUFFER_SIZE >= 0)
                                        {
                                            params.append(" -f ");
                                            params.append(lexical_cast<string >(protocol.TCP_BUFFER_SIZE));
                                        }
                                    else
                                        {
                                            params.append(" -f ");
                                            params.append(lexical_cast<string >(DEFAULT_BUFFSIZE));
                                        }
                                }

                            if (optimize && manualConfigExists == false)
                                {
                                    params.append(" -h ");
                                    params.append(lexical_cast<string >(Timeout));
                                }
                            else
                                {
                                    if (protocol.URLCOPY_TX_TO >= 0)
                                        {
                                            params.append(" -h ");
                                            params.append(lexical_cast<string >(protocol.URLCOPY_TX_TO));
                                        }
                                    else
                                        {
                                            params.append(" -h ");
                                            params.append(lexical_cast<string >(DEFAULT_TIMEOUT));
                                        }
                                }
                            if (std::string(temp->SOURCE_SPACE_TOKEN).length() > 0)
                                {
                                    params.append(" -k ");
                                    params.append(temp->SOURCE_SPACE_TOKEN);
                                }
                            if (std::string(temp->DEST_SPACE_TOKEN).length() > 0)
                                {
                                    params.append(" -j ");
                                    params.append(temp->DEST_SPACE_TOKEN);
                                }

                            if (temp->PIN_LIFETIME > 0)
                                {
                                    params.append(" -t ");
                                    params.append(lexical_cast<string >(temp->PIN_LIFETIME));
                                }

                            if (temp->BRINGONLINE > 0)
                                {
                                    params.append(" -H ");
                                    params.append(lexical_cast<string >(temp->BRINGONLINE));
                                }

                            if (temp->USER_FILESIZE > 0)
                                {
                                    params.append(" -I ");
                                    params.append(lexical_cast<string >(temp->USER_FILESIZE));
                                }

                            if (temp->FILE_METADATA.length() > 0)
                                {
                                    params.append(" -K ");
                                    params.append(prepareMetadataString(temp->FILE_METADATA));
                                }

                            if (temp->JOB_METADATA.length() > 0)
                                {
                                    params.append(" -J ");
                                    params.append(prepareMetadataString(temp->JOB_METADATA));
                                }

                            if (temp->BRINGONLINE_TOKEN.length() > 0)
                                {
                                    params.append(" -L ");
                                    params.append(temp->BRINGONLINE_TOKEN);
                                }

                            params.append(" -M ");
                            params.append(infosys);

                            bool ready = DBSingleton::instance().getDBObjectInstance()->isFileReadyState(temp->FILE_ID);

                            if (ready)
                                {
                                    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Transfer params: " << ftsHostname << " "  << cmd << " " << params << commit;
                                    ExecuteProcess *pr = new ExecuteProcess(cmd, params);
                                    if (pr)
                                        {
                                            /*check if fork/execvp failed, */
                                            if (-1 == pr->executeProcessShell())
                                                {
                                                    FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Transfer failed to spawn " <<  temp->JOB_ID << "  " << temp->FILE_ID << commit;
                                                    DBSingleton::instance().getDBObjectInstance()->forkFailedRevertState(temp->JOB_ID, temp->FILE_ID);
                                                }
                                            else
                                                {
                                                    DBSingleton::instance().getDBObjectInstance()->updateFileTransferStatus(0.0, temp->JOB_ID, temp->FILE_ID, "ACTIVE", "",(int) pr->getPid(), 0, 0);
                                                    DBSingleton::instance().getDBObjectInstance()->updateJobTransferStatus(0, temp->JOB_ID, "ACTIVE");
                                                    SingleTrStateInstance::instance().sendStateMessage(temp->JOB_ID, temp->FILE_ID);
                                                    DBSingleton::instance().getDBObjectInstance()->setPid(temp->JOB_ID, temp->FILE_ID, pr->getPid());
                                                    struct message_updater msg;
                                                    strcpy(msg.job_id, std::string(temp->JOB_ID).c_str());
                                                    msg.file_id = temp->FILE_ID;
                                                    msg.process_id = (int) pr->getPid();
                                                    msg.timestamp = milliseconds_since_epoch();
                                                    ThreadSafeList::get_instance().push_back(msg);
                                                }
                                            delete pr;
                                        }
                                }
                            params.clear();
                        }
                    else
                        {
                            notScheduled.insert(make_pair(source_hostname, destin_hostname));
                        }
                }
            catch(std::exception& e)
                {
                    FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Process thread exception " << e.what() <<  commit;
                }
            catch(...)
                {
                    FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Process thread exception unknown" <<  commit;
                }
        }

    active = false;
}

} /* namespace server */
} /* namespace fts3 */

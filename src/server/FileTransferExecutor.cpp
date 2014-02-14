/*
 * FileTransferExecutor.cpp
 *
 *  Created on: Aug 9, 2013
 *      Author: simonm
 */

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


FileTransferExecutor::FileTransferExecutor(TransferFiles* tf, TransferFileHandler& tfh, bool optimize, bool monitoringMsg, string infosys, string ftsHostName) :
    tf(tf),
    tfh(tfh),
    optimize(optimize),
    monitoringMsg(monitoringMsg),
    infosys(infosys),
    ftsHostName(ftsHostName),
    db(DBSingleton::instance().getDBObjectInstance())
{
}

FileTransferExecutor::~FileTransferExecutor()
{

}

string FileTransferExecutor::prepareMetadataString(std::string text)
{
    text = boost::replace_all_copy(text, " ", "?");
    text = boost::replace_all_copy(text, "\"", "\\\"");
    return text;
}

int FileTransferExecutor::execute()
{
    int scheduled = 0;

    try
        {
            string source_hostname = tf->SOURCE_SE;
            string destin_hostname = tf->DEST_SE;

            // if the pair was already checked and not scheduled skip it
            if (notScheduled.count(make_pair(source_hostname, destin_hostname))) return scheduled;

            /*check if manual config exist for this pair and vo*/

            vector< boost::shared_ptr<ShareConfig> > cfgs;

            ConfigurationAssigner cfgAssigner(tf.get());
            cfgAssigner.assign(cfgs);

            SeProtocolConfig protocol;

            int BufSize = 0;
            int StreamsperFile = 0;
            int Timeout = 0;
            bool manualProtocol = false;

            std::stringstream internalParams;

            bool manualConfigExists = false;

            if (optimize)
                {
                    optional<ProtocolResolver::protocol> p = ProtocolResolver::getUserDefinedProtocol(tf.get());

                    if (p.is_initialized())
                        {
                            BufSize = (*p).tcp_buffer_size;
                            StreamsperFile = (*p).nostreams;
                            Timeout = (*p).urlcopy_tx_to;
                            manualProtocol = true;
                        }
                    else
                        {
                            OptimizerSample* opt_config = new OptimizerSample();
                            db->fetchOptimizationConfig2(opt_config, source_hostname, destin_hostname);
                            BufSize = opt_config->getBufSize();
                            StreamsperFile = opt_config->getStreamsperFile();
                            Timeout = opt_config->getTimeout();
                            delete opt_config;
                            opt_config = NULL;
                        }
                }

            FileTransferScheduler scheduler(
                tf.get(),
                cfgs,
                tfh.getDestinations(source_hostname),
                tfh.getSources(destin_hostname),
                tfh.getDestinationsVos(source_hostname),
                tfh.getSourcesVos(destin_hostname)

            );

            if (scheduler.schedule(optimize))   /*SET TO READY STATE WHEN TRUE*/
                {
                    scheduled = 1;

                    SingleTrStateInstance::instance().sendStateMessage(tf->JOB_ID, tf->FILE_ID);
                    bool isAutoTuned = false;

                    if (optimize && cfgs.empty())
                        {
                            /* Not used for now, please do not remove
                                        db->setAllowed(tf->JOB_ID, tf->FILE_ID, source_hostname, destin_hostname, StreamsperFile, Timeout, BufSize);
                            */
                            if (manualProtocol == true)
                                {
                                    internalParams << "nostreams:" << StreamsperFile << ",timeout:" << Timeout << ",buffersize:" << BufSize;
                                }
                            else
                                {
                                    internalParams << "nostreams:" << DEFAULT_NOSTREAMS << ",timeout:" << DEFAULT_TIMEOUT << ",buffersize:" << DEFAULT_BUFFSIZE;
                                }
                        }
                    else
                        {
                            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Check link config for: " << source_hostname << " -> " << destin_hostname << commit;
                            ProtocolResolver resolver(tf.get(), cfgs);
                            bool protocolExists = resolver.resolve();
                            if (protocolExists == true)
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
                            else if (manualProtocol == true)
                                {
                                    internalParams << "nostreams:" << StreamsperFile << ",timeout:" << Timeout << ",buffersize:" << BufSize;
                                }
                            else
                                {
                                    internalParams << "nostreams:" << DEFAULT_NOSTREAMS << ",timeout:" << DEFAULT_TIMEOUT << ",buffersize:" << DEFAULT_BUFFSIZE;
                                }

                            if (resolver.isAuto())
                                {
                                    isAutoTuned = true;
                                    /* Not used for now, please do not remove
                                                    db->setAllowed(
                                                        tf->JOB_ID,
                                                        tf->FILE_ID,
                                                        source_hostname,
                                                        destin_hostname,
                                                        resolver.getNoStreams(),
                                                        resolver.getNoTxActiveTo(),
                                                        resolver.getTcpBufferSize()
                                                    );
                                    */
                                }
                            else
                                {
                                    db->setAllowedNoOptimize(
                                        tf->JOB_ID,
                                        tf->FILE_ID,
                                        internalParams.str()
                                    );
                                }
                        }

                    string proxy_file = get_proxy_cert(
                                            tf->DN, // user_dn
                                            tf->CRED_ID, // user_cred
                                            tf->VO_NAME, // vo_name
                                            "",
                                            "", // assoc_service
                                            "", // assoc_service_type
                                            false,
                                            ""
                                        );

                    //disable for now, remove later
                    string sourceSiteName = ""; //siteResolver.getSiteName(tf->SOURCE_SURL);
                    string destSiteName = ""; //siteResolver.getSiteName(tf->DEST_SURL);

                    bool debug = db->getDebugMode(source_hostname, destin_hostname);

                    string params;

                    if (debug == true)
                        {
                            params.append(" -F ");
                        }

                    if (manualConfigExists || manualProtocol)
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

                    if (std::string(tf->CHECKSUM).length() > 0)   //checksum
                        {
                            params.append(" -z ");
                            params.append(tf->CHECKSUM);
                        }
                    if (std::string(tf->CHECKSUM_METHOD).length() > 0)   //checksum
                        {
                            params.append(" -A ");
                            params.append(tf->CHECKSUM_METHOD);
                        }
                    params.append(" -b ");
                    params.append(tf->SOURCE_SURL);
                    params.append(" -c ");
                    params.append(tf->DEST_SURL);
                    params.append(" -a ");
                    params.append(tf->JOB_ID);
                    params.append(" -B ");
                    params.append(lexical_cast<string >(tf->FILE_ID));
                    params.append(" -C ");
                    params.append(tf->VO_NAME);
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
                    if (std::string(tf->OVERWRITE).length() > 0)
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
                    if (std::string(tf->SOURCE_SPACE_TOKEN).length() > 0)
                        {
                            params.append(" -k ");
                            params.append(tf->SOURCE_SPACE_TOKEN);
                        }
                    if (std::string(tf->DEST_SPACE_TOKEN).length() > 0)
                        {
                            params.append(" -j ");
                            params.append(tf->DEST_SPACE_TOKEN);
                        }

                    if (tf->PIN_LIFETIME > 0)
                        {
                            params.append(" -t ");
                            params.append(lexical_cast<string >(tf->PIN_LIFETIME));
                        }

                    if (tf->BRINGONLINE > 0)
                        {
                            params.append(" -H ");
                            params.append(lexical_cast<string >(tf->BRINGONLINE));
                        }

                    if (tf->USER_FILESIZE > 0)
                        {
                            params.append(" -I ");
                            params.append(lexical_cast<string >(tf->USER_FILESIZE));
                        }

                    if (tf->FILE_METADATA.length() > 0)
                        {
                            params.append(" -K ");
                            params.append(prepareMetadataString(tf->FILE_METADATA));
                        }

                    if (tf->JOB_METADATA.length() > 0)
                        {
                            params.append(" -J ");
                            params.append(prepareMetadataString(tf->JOB_METADATA));
                        }

                    if (tf->BRINGONLINE_TOKEN.length() > 0)
                        {
                            params.append(" -L ");
                            params.append(tf->BRINGONLINE_TOKEN);
                        }

                    if(infosys.length() > 0)
                        {
                            params.append(" -M ");
                            params.append(infosys);
                        }

                    bool ready = db->isFileReadyState(tf->FILE_ID);

                    if (ready)
                        {
                            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Transfer params: " << cmd << " " << params << commit;
                            ExecuteProcess *pr = new ExecuteProcess(cmd, params);
                            if (pr)
                                {
                                    /*check if fork/execvp failed, */
                                    if (-1 == pr->executeProcessShell())
                                        {
                                            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Transfer failed to spawn " <<  tf->JOB_ID << "  " << tf->FILE_ID << commit;
                                            db->forkFailedRevertState(tf->JOB_ID, tf->FILE_ID);
                                        }
                                    else
                                        {
                                            db->updateFileTransferStatus(0.0, tf->JOB_ID, tf->FILE_ID, "ACTIVE", "",(int) pr->getPid(), 0, 0, false);
                                            db->updateJobTransferStatus(tf->JOB_ID, "ACTIVE");
                                            SingleTrStateInstance::instance().sendStateMessage(tf->JOB_ID, tf->FILE_ID);
                                            struct message_updater msg;
                                            strncpy(msg.job_id, std::string(tf->JOB_ID).c_str(), sizeof(msg.job_id));
                                            msg.job_id[sizeof(msg.job_id) - 1] = '\0';
                                            msg.file_id = tf->FILE_ID;
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

    return scheduled;
}

} /* namespace server */
} /* namespace fts3 */

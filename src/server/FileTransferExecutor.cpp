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
#include "UserProxyEnv.h"
#include "DelegCred.h"
#include "CredService.h"
#include "ws/SingleTrStateInstance.h"

#include "cred/cred-utility.h"

extern bool stopThreads;


namespace fts3
{

namespace server
{

const string FileTransferExecutor::cmd = "fts_url_copy";


FileTransferExecutor::FileTransferExecutor(TransferFiles& tf, TransferFileHandler& tfh, bool monitoringMsg, string infosys, string ftsHostName) :
    tf(tf),
    tfh(tfh),
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

std::string FileTransferExecutor::generateProxy(const std::string& dn, const std::string& dlg_id)
{
    boost::scoped_ptr<DelegCred> delegCredPtr(new DelegCred);
    return delegCredPtr->getFileName(dn, dlg_id);
}

bool FileTransferExecutor::checkValidProxy(const std::string& filename, std::string& message)
{
    boost::scoped_ptr<DelegCred> delegCredPtr(new DelegCred);
    return delegCredPtr->isValidProxy(filename, message);
}

int FileTransferExecutor::execute()
{
    int scheduled = 0;

    if (tf.FILE_ID == 0)
        return 0;

    try
        {
            string source_hostname = tf.SOURCE_SE;
            string destin_hostname = tf.DEST_SE;
            string params;

            // if the pair was already checked and not scheduled skip it
            if (notScheduled.count(make_pair(source_hostname, destin_hostname))) return scheduled;

            /*check if manual config exist for this pair and vo*/

            vector< boost::shared_ptr<ShareConfig> > cfgs;

            ConfigurationAssigner cfgAssigner(tf);
            cfgAssigner.assign(cfgs);

            SeProtocolConfig protocol;

            int BufSize = 0;
            int StreamsperFile = 0;
            int Timeout = 0;
            bool manualProtocol = false;

            bool manualConfigExists = false;

            optional<ProtocolResolver::protocol> p = ProtocolResolver::getUserDefinedProtocol(tf);

            if (p.is_initialized())
                {
                    BufSize = (*p).tcp_buffer_size;
                    StreamsperFile = (*p).nostreams;
                    Timeout = (*p).urlcopy_tx_to;
                    manualProtocol = true;
                }
            else
                {
                    BufSize = db->getBufferOptimization();
                    StreamsperFile = db->getStreamsOptimization(source_hostname, destin_hostname);
                    Timeout = db->getGlobalTimeout();
                    if(Timeout == 0)
                        Timeout = DEFAULT_TIMEOUT;
                    else
                        params.append(" -Z ");

                    int secPerMB = db->getSecPerMb();
                    if(secPerMB > 0)
                        {
                            params.append(" -V ");
                            params.append(lexical_cast<string >(secPerMB));
                        }
                }

            FileTransferScheduler scheduler(
                tf,
                cfgs,
                tfh.getDestinations(source_hostname),
                tfh.getSources(destin_hostname),
                tfh.getDestinationsVos(source_hostname),
                tfh.getSourcesVos(destin_hostname)
            );

            if (scheduler.schedule())   /*SET TO READY STATE WHEN TRUE*/
                {
                    scheduled = 1;

                    SingleTrStateInstance::instance().sendStateMessage(tf.JOB_ID, tf.FILE_ID);
                    bool isAutoTuned = false;

                    if (!cfgs.empty())
                        {
                            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Check link config for: " << source_hostname << " -> " << destin_hostname << commit;
                            ProtocolResolver resolver(tf, cfgs);
                            bool protocolExists = resolver.resolve();
                            if (protocolExists)
                                {
                                    manualConfigExists = true;
                                    protocol.NOSTREAMS = resolver.getNoStreams();
                                    protocol.NO_TX_ACTIVITY_TO = resolver.getNoTxActiveTo();
                                    protocol.TCP_BUFFER_SIZE = resolver.getTcpBufferSize();
                                    protocol.URLCOPY_TX_TO = resolver.getUrlCopyTxTo();
                                }

                            if (resolver.isAuto())
                                {
                                    isAutoTuned = true;
                                }
                        }

                    //get the proxy
                    std::string proxy_file = generateProxy(tf.DN, tf.CRED_ID);
                    std::string message;
                    if(false == checkValidProxy(proxy_file, message))
                        {
                            proxy_file = get_proxy_cert(
                                             tf.DN, // user_dn
                                             tf.CRED_ID, // user_cred
                                             tf.VO_NAME, // vo_name
                                             "",
                                             "", // assoc_service
                                             "", // assoc_service_type
                                             false,
                                             ""
                                         );
                        }


                    params.append(" -Y ");
                    params.append(prepareMetadataString(tf.DN));

                    //disable for now, remove later
                    string sourceSiteName = ""; //siteResolver.getSiteName(tf.SOURCE_SURL);
                    string destSiteName = ""; //siteResolver.getSiteName(tf.DEST_SURL);

                    bool debug = db->getDebugMode(source_hostname, destin_hostname);

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

                    if (std::string(tf.CHECKSUM).length() > 0)   //checksum
                        {
                            params.append(" -z ");
                            params.append(tf.CHECKSUM);
                        }
                    if (std::string(tf.CHECKSUM_METHOD).length() > 0)   //checksum
                        {
                            params.append(" -A ");
                            params.append(tf.CHECKSUM_METHOD);
                        }
                    params.append(" -b ");
                    params.append(tf.SOURCE_SURL);
                    params.append(" -c ");
                    params.append(tf.DEST_SURL);
                    params.append(" -a ");
                    params.append(tf.JOB_ID);
                    params.append(" -B ");
                    params.append(lexical_cast<string >(tf.FILE_ID));
                    params.append(" -C ");
                    params.append(tf.VO_NAME);
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
                    if (std::string(tf.OVERWRITE).length() > 0)
                        {
                            params.append(" -d ");
                        }
                    if (!manualConfigExists)
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

                    if (!manualConfigExists)
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

                    if (!manualConfigExists)
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
                    if (std::string(tf.SOURCE_SPACE_TOKEN).length() > 0)
                        {
                            params.append(" -k ");
                            params.append(tf.SOURCE_SPACE_TOKEN);
                        }
                    if (std::string(tf.DEST_SPACE_TOKEN).length() > 0)
                        {
                            params.append(" -j ");
                            params.append(tf.DEST_SPACE_TOKEN);
                        }

                    if (tf.PIN_LIFETIME > 0)
                        {
                            params.append(" -t ");
                            params.append(lexical_cast<string >(tf.PIN_LIFETIME));
                        }

                    if (tf.BRINGONLINE > 0)
                        {
                            params.append(" -H ");
                            params.append(lexical_cast<string >(tf.BRINGONLINE));
                        }

                    if (tf.USER_FILESIZE > 0)
                        {
                            params.append(" -I ");
                            params.append(lexical_cast<string >(tf.USER_FILESIZE));
                        }

                    if (tf.FILE_METADATA.length() > 0)
                        {
                            params.append(" -K ");
                            params.append(prepareMetadataString(tf.FILE_METADATA));
                        }

                    if (tf.JOB_METADATA.length() > 0)
                        {
                            params.append(" -J ");
                            params.append(prepareMetadataString(tf.JOB_METADATA));
                        }

                    if (tf.BRINGONLINE_TOKEN.length() > 0)
                        {
                            params.append(" -L ");
                            params.append(tf.BRINGONLINE_TOKEN);
                        }

                    if(infosys.length() > 0)
                        {
                            params.append(" -M ");
                            params.append(infosys);
                        }

                    bool udt = db->isProtocolUDT(source_hostname, destin_hostname);

                    if(udt)
                        {
                            params.append(" -U ");
                        }


                    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Transfer params: " << cmd << " " << params << commit;
                    ExecuteProcess pr(cmd, params);

                    /*check if fork/execvp failed, */
                    std::string forkMessage;
		    bool failed = false;
                    if (-1 == pr.executeProcessShell(forkMessage))
                        {
                            if(forkMessage.empty())
                                {
				    failed = true;
                                    FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Transfer failed to fork " <<  tf.JOB_ID << "  " << tf.FILE_ID << commit;
                                    db->updateFileTransferStatus(0.0, tf.JOB_ID, tf.FILE_ID, "FAILED", "Transfer failed to fork, check fts3server.log for more details",(int) pr.getPid(), 0, 0, false);                                   
				    db->updateJobTransferStatus(tf.JOB_ID, "FAILED",0); 
                                }
                            else
                                {
				    failed = true;				
                                    FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Transfer failed to fork " <<  forkMessage << "   " <<  tf.JOB_ID << "  " << tf.FILE_ID << commit;
                                    db->updateFileTransferStatus(0.0, tf.JOB_ID, tf.FILE_ID, "FAILED", "Transfer failed to fork, check fts3server.log for more details",(int) pr.getPid(), 0, 0, false);                                    
				    db->updateJobTransferStatus(tf.JOB_ID, "FAILED",0); 
                                }
                        }
                    else
                        {
                            db->updateFileTransferStatus(0.0, tf.JOB_ID, tf.FILE_ID, "ACTIVE", "",(int) pr.getPid(), 0, 0, false);     
			    db->updateJobTransferStatus(tf.JOB_ID, "ACTIVE",0);                        
			}

                            SingleTrStateInstance::instance().sendStateMessage(tf.JOB_ID, tf.FILE_ID);
                            struct message_updater msg;
                            strncpy(msg.job_id, std::string(tf.JOB_ID).c_str(), sizeof(msg.job_id));
                            msg.job_id[sizeof(msg.job_id) - 1] = '\0';
                            msg.file_id = tf.FILE_ID;
                            msg.process_id = (int) pr.getPid();
                            msg.timestamp = milliseconds_since_epoch();
			    
			    if(!failed) //only set watcher when the file has started
                            	ThreadSafeList::get_instance().push_back(msg);

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

/*
 *	Copyright notice:
 *	Copyright © Members of the EMI Collaboration, 2010.
 *
 *	See www.eu-emi.eu for details on the copyright holders
 *
 *	Licensed under the Apache License, Version 2.0 (the "License");
 *	you may not use soap file except in compliance with the License.
 *	You may obtain a copy of the License at
 *
 *		http://www.apache.org/licenses/LICENSE-2.0
 *
 *	Unless required by applicable law or agreed to in writing, software
 *	distributed under the License is distributed on an "AS IS" BASIS,
 *	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implcfgied.
 *	See the License for the specific language governing permissions and
 *	limitations under the License.
 */

#include "ws-ifce/gsoap/gsoap_stubs.h"

#include "ConfigurationHandler.h"
#include "Blacklister.h"
#include "ws/AuthorizationManager.h"
#include "ws/CGsiAdapter.h"

#include "db/generic/SingleDbInstance.h"

#include "common/logger.h"
#include "common/error.h"

#include "DrainMode.h"

#include <vector>
#include <string>
#include <set>
#include <exception>
#include <utility>

#include <boost/regex.hpp>
#include <boost/algorithm/string.hpp>

using namespace std;
using namespace db;
using namespace fts3::common;
using namespace boost;
using namespace boost::algorithm;
using namespace fts3::ws;


int fts3::implcfg__setConfiguration(soap* soap, config__Configuration *_configuration, struct implcfg__setConfigurationResponse &response)
{

//	FTS3_COMMON_LOGGER_NEWLOG (INFO) << "Handling 'setConfiguration' request" << commit;

    vector<string>& cfgs = _configuration->cfg;
    vector<string>::iterator it;

    try
        {
            CGsiAdapter cgsi(soap);
            string dn = cgsi.getClientDn();

            ConfigurationHandler handler (dn);
            for(it = cfgs.begin(); it < cfgs.end(); it++)
                {
                    handler.parse(*it);
                    // authorize each configuration operation separately for each se/se group
                    AuthorizationManager::getInstance().authorize(
                        soap,
                        AuthorizationManager::CONFIG,
                        AuthorizationManager::dummy
                    );
                    handler.add();
                }
        }
    catch(Err& ex)
        {

            FTS3_COMMON_LOGGER_NEWLOG (ERR) << "An exception has been caught: " << ex.what() << commit;
            soap_receiver_fault(soap, ex.what(), "ConfigurationException");
            return SOAP_FAULT;
        }
    catch(...)
        {

            FTS3_COMMON_LOGGER_NEWLOG (ERR) << "A std::exception has been caught: setConfiguration"  << commit;
            soap_receiver_fault(soap, "setConfiguration", "ConfigurationException");
            return SOAP_FAULT;

        }
    return SOAP_OK;
}

/* ---------------------------------------------------------------------- */

int fts3::implcfg__getConfiguration(soap* soap, string all, string name, string source, string destination, implcfg__getConfigurationResponse & response)
{
    response.configuration = soap_new_config__Configuration(soap, -1);
    try
        {
            CGsiAdapter cgsi(soap);
            string dn = cgsi.getClientDn();

            bool allcfgs = source.empty() && destination.empty();
            bool standalone = !source.empty() && destination.empty();
            bool pair = !source.empty() && !destination.empty();
            bool symbolic_name = !name.empty();

            ConfigurationHandler handler (dn);
            if (allcfgs)
                {
                    response.configuration->cfg = handler.get();
                }
            else if (standalone)
                {
                    // the user is querying for activity share configuration
                    if (all == "vo")
                        {
                            response.configuration->cfg.push_back(handler.getVo(source));
                        }
                    // the user is querying for all configuration regarding given SE
                    else if (all == "all")
                        {
                            response.configuration->cfg = handler.getAll(source);
                        }
                    // standard standalone configuration
                    else
                        {
                            response.configuration->cfg.push_back(handler.get(source));
                        }
                }
            else if (pair)
                {
                    response.configuration->cfg.push_back(handler.getPair(source, destination));
                }
            else if (symbolic_name)
                {
                    response.configuration->cfg.push_back(handler.getPair(name));
                }
            else
                {
                    throw Err_Custom("Wrongly specified parameters, either both the source and destination have to be specified or the configuration name!");
                }

        }
    catch(Err& ex)
        {

            FTS3_COMMON_LOGGER_NEWLOG (ERR) << "An exception has been caught: " << ex.what() << commit;
            soap_receiver_fault(soap, ex.what(), "ConfigurationException");
            return SOAP_FAULT;
        }
    catch(...)
        {

            FTS3_COMMON_LOGGER_NEWLOG (ERR) << "A std::exception has been caught: getConfiguration"  << commit;
            soap_receiver_fault(soap, "getConfiguration", "ConfigurationException");
            return SOAP_FAULT;

        }

    return SOAP_OK;
}

/* ---------------------------------------------------------------------- */

int fts3::implcfg__delConfiguration(soap* soap, config__Configuration *_configuration, struct implcfg__delConfigurationResponse &_param_11)
{

//	FTS3_COMMON_LOGGER_NEWLOG (INFO) << "Handling 'delConfiguration' request" << commit;

    vector<string>& cfgs = _configuration->cfg;
    vector<string>::iterator it;

    try
        {
            CGsiAdapter cgsi(soap);
            string dn = cgsi.getClientDn();

            ConfigurationHandler handler (dn);
            for(it = cfgs.begin(); it < cfgs.end(); it++)
                {
                    handler.parse(*it);
                    // authorize each configuration operation separately for each se/se group
                    AuthorizationManager::getInstance().authorize(
                        soap,
                        AuthorizationManager::CONFIG,
                        AuthorizationManager::dummy
                    );
                    handler.del();
                }

        }
    catch(Err& ex)
        {

            FTS3_COMMON_LOGGER_NEWLOG (ERR) << "An exception has been caught: " << ex.what() << commit;
            soap_receiver_fault(soap, ex.what(), "ConfigurationException");
            return SOAP_FAULT;
        }
    catch(...)
        {

            FTS3_COMMON_LOGGER_NEWLOG (ERR) << "A std::exception has been caught: delConfiguration" << commit;
            soap_receiver_fault(soap, "delConfiguration", "ConfigurationException");
            return SOAP_FAULT;

        }

    return SOAP_OK;
}

/* ---------------------------------------------------------------------- */

int fts3::implcfg__doDrain(soap* ctx, bool drain, struct implcfg__doDrainResponse &_param_13)
{
    try
        {
            // authorize
            AuthorizationManager::getInstance().authorize(
                ctx,
                AuthorizationManager::CONFIG,
                AuthorizationManager::dummy
            );

            // get user dn
            CGsiAdapter cgsi(ctx);
            string dn = cgsi.getClientDn();

            // prepare the command for audit
            stringstream cmd;
            cmd << "fts-config-set --drain " << (drain ? "on" : "off");

            FTS3_COMMON_LOGGER_NEWLOG (INFO) << "Turning " << (drain ? "on" : "off") << " the drain mode" << commit;
            DrainMode::getInstance() = drain;
            // audit the operation
            DBSingleton::instance().getDBObjectInstance()->auditConfiguration(dn, cmd.str(), "drain");
        }
    catch(Err& ex)
        {

            FTS3_COMMON_LOGGER_NEWLOG (ERR) << "An exception has been caught: " << ex.what() << commit;
            soap_receiver_fault(ctx, ex.what(), "TransferException");

            return SOAP_FAULT;
        }
    catch (...)
        {
            FTS3_COMMON_LOGGER_NEWLOG (ERR) << "An exception has been thrown, the drain mode cannot be set"  << commit;
            return SOAP_FAULT;
        }

    return SOAP_OK;
}

/* ---------------------------------------------------------------------- */

int fts3::implcfg__showUserDn(soap* ctx, bool show, implcfg__showUserDnResponse& resp)
{
    try
        {
            // authorize
            AuthorizationManager::getInstance().authorize(
                ctx,
                AuthorizationManager::CONFIG,
                AuthorizationManager::dummy
            );

            // get user dn
            CGsiAdapter cgsi(ctx);
            string dn = cgsi.getClientDn();

            // prepare the command for audit
            stringstream cmd;
            cmd << "fts-config-set --drain " << (show ? "on" : "off");

            FTS3_COMMON_LOGGER_NEWLOG (INFO) << "Turning " << (show ? "on" : "off") << " the show-user-dn mode" << commit;

            // TODO DB
            DBSingleton::instance().getDBObjectInstance()->setShowUserDn(show);

            // audit the operation
            DBSingleton::instance().getDBObjectInstance()->auditConfiguration(dn, cmd.str(), "show-user-dn");
        }
    catch(Err& ex)
        {

            FTS3_COMMON_LOGGER_NEWLOG (ERR) << "An exception has been caught: " << ex.what() << commit;
            soap_receiver_fault(ctx, ex.what(), "TransferException");

            return SOAP_FAULT;
        }
    catch (...)
        {
            FTS3_COMMON_LOGGER_NEWLOG (ERR) << "An exception has been thrown, the drain mode cannot be set"  << commit;
            return SOAP_FAULT;
        }

    return SOAP_OK;
}

/* ---------------------------------------------------------------------- */

int fts3::implcfg__setRetry(soap* ctx, std::string vo, int retry, implcfg__setRetryResponse& _resp)
{

    try
        {
            // authorize
            AuthorizationManager::getInstance().authorize(
                ctx,
                AuthorizationManager::CONFIG,
                AuthorizationManager::dummy
            );

            // get user dn
            CGsiAdapter cgsi(ctx);
            string dn = cgsi.getClientDn();

            // prepare the command for audit
            stringstream cmd;
            cmd << "fts-config-set --retry " << vo << " " << retry;

            // audit the operation
            DBSingleton::instance().getDBObjectInstance()->auditConfiguration(dn, cmd.str(), "retry");

            // set the number of retries
            DBSingleton::instance().getDBObjectInstance()->setRetry(retry, vo);

            // log it
            FTS3_COMMON_LOGGER_NEWLOG (INFO) << "User: " << dn << " had set the retry value to " << retry << " for VO " << vo << commit;

        }
    catch(Err& ex)
        {

            FTS3_COMMON_LOGGER_NEWLOG (ERR) << "An exception has been caught: " << ex.what() << commit;
            soap_receiver_fault(ctx, ex.what(), "TransferException");

            return SOAP_FAULT;
        }
    catch (...)
        {
            FTS3_COMMON_LOGGER_NEWLOG (ERR) << "An exception has been thrown, the number of retries cannot be set"  << commit;
            return SOAP_FAULT;
        }

    return SOAP_OK;
}

/* ---------------------------------------------------------------------- */

int fts3::implcfg__setOptimizerMode(soap* ctx, int optimizer_mode, implcfg__setOptimizerModeResponse &resp)
{
    try
        {
            // authorize
            AuthorizationManager::getInstance().authorize(
                ctx,
                AuthorizationManager::CONFIG,
                AuthorizationManager::dummy
            );

            // get user dn
            CGsiAdapter cgsi(ctx);
            string dn = cgsi.getClientDn();

            // prepare the command for audit
            stringstream cmd;
            cmd << "fts-config-set --optimizer-mode " << optimizer_mode;

            // audit the operation
            DBSingleton::instance().getDBObjectInstance()->auditConfiguration(dn, cmd.str(), "optimizer mode");

            // set the number of retries
            DBSingleton::instance().getDBObjectInstance()->setOptimizerMode(optimizer_mode);

            // log it
            FTS3_COMMON_LOGGER_NEWLOG (INFO) << "User: " << dn << " had set the optmizer mode to " << optimizer_mode << commit;

        }
    catch(Err& ex)
        {

            FTS3_COMMON_LOGGER_NEWLOG (ERR) << "An exception has been caught: " << ex.what() << commit;
            soap_receiver_fault(ctx, ex.what(), "TransferException");

            return SOAP_FAULT;
        }
    catch (...)
        {
            FTS3_COMMON_LOGGER_NEWLOG (ERR) << "An exception has been thrown, setOptimizerMode cannot be set"  << commit;
            return SOAP_FAULT;
        }

    return SOAP_OK;
}

/* ---------------------------------------------------------------------- */

int fts3::implcfg__setQueueTimeout(soap* ctx, unsigned int timeout, implcfg__setQueueTimeoutResponse& resp)
{

    try
        {
            // authorize
            AuthorizationManager::getInstance().authorize(
                ctx,
                AuthorizationManager::CONFIG,
                AuthorizationManager::dummy
            );

            // get user dn
            CGsiAdapter cgsi(ctx);
            string dn = cgsi.getClientDn();

            // prepare the command for audit
            stringstream cmd;
            cmd << "fts-config-set --queue-timeout " << timeout;

            // audit the operation
            DBSingleton::instance().getDBObjectInstance()->auditConfiguration(dn, cmd.str(), "queue-timeout");

            // set the number of retries
            DBSingleton::instance().getDBObjectInstance()->setMaxTimeInQueue(timeout);

            // log it
            FTS3_COMMON_LOGGER_NEWLOG (INFO) << "User: " << dn << " had set the maximum timeout in the queue to " << timeout << commit;

        }
    catch(Err& ex)
        {

            FTS3_COMMON_LOGGER_NEWLOG (ERR) << "An exception has been caught: " << ex.what() << commit;
            soap_receiver_fault(ctx, ex.what(), "TransferException");

            return SOAP_FAULT;
        }
    catch (...)
        {
            FTS3_COMMON_LOGGER_NEWLOG (ERR) << "An exception has been thrown, setQueueTimeout cannot be set"  << commit;
            return SOAP_FAULT;
        }

    return SOAP_OK;
}

int fts3::implcfg__setBringOnline(soap* ctx, config__BringOnline *bring_online, implcfg__setBringOnlineResponse &resp)
{

    try
        {
            // authorize
            AuthorizationManager::getInstance().authorize(
                ctx,
                AuthorizationManager::CONFIG,
                AuthorizationManager::dummy
            );

            try
                {
                    AuthorizationManager::getInstance().authorize(
                        ctx,
                        AuthorizationManager::DELEG,
                        AuthorizationManager::dummy
                    );
                }
            catch (Err& ex)
                {
                    throw Err_Custom("Authorization failed, a host certificate has been used to set max number of concurrent staging operations!");
                }

            // get user VO and DN
            CGsiAdapter cgsi(ctx);
            string vo = cgsi.getClientVo();
            string dn = cgsi.getClientDn();

            vector<config__BringOnlineTriplet*>& v = bring_online->boElem;
            vector<config__BringOnlineTriplet*>::iterator it;

            for (it = v.begin(); it != v.end(); it++)
                {

                    config__BringOnlineTriplet* triplet = *it;

                    DBSingleton::instance().getDBObjectInstance()->setMaxStageOp(
                        triplet->se,
                        triplet->vo.empty() ? vo : triplet->vo,
                        triplet->value,
                        triplet->operation
                    );

                    // log it
                    FTS3_COMMON_LOGGER_NEWLOG (INFO)
                            << "User: "
                            << dn
                            << " had set the maximum number of concurrent " << triplet->operation << " operations for se : "
                            << triplet->se << " and vo : " << vo
                            << " to "
                            << triplet->value
                            << commit;
                }

        }
    catch(Err& ex)
        {

            FTS3_COMMON_LOGGER_NEWLOG (ERR) << "An exception has been caught: " << ex.what() << commit;
            soap_receiver_fault(ctx, ex.what(), "TransferException");

            return SOAP_FAULT;
        }
    catch (...)
        {
            FTS3_COMMON_LOGGER_NEWLOG (ERR) << "An exception has been thrown, the bring online limit"  << commit;
            return SOAP_FAULT;
        }

    return SOAP_OK;
}

int fts3::implcfg__setBandwidthLimit(soap* ctx, fts3::config__BandwidthLimit* limit, fts3::implcfg__setBandwidthLimitResponse& resp)
{
    try
        {
            // authorize
            AuthorizationManager::getInstance().authorize(
                ctx,
                AuthorizationManager::CONFIG,
                AuthorizationManager::dummy
            );

            CGsiAdapter cgsi(ctx);
            string vo = cgsi.getClientVo();
            string dn = cgsi.getClientDn();

            vector<config__BandwidthLimitPair*>::iterator it;
            for (it = limit->blElem.begin(); it != limit->blElem.end(); ++it)
                {
                    config__BandwidthLimitPair* pair = *it;

                    if (!pair->source.empty() && !pair->dest.empty())
                        throw Err_Custom("Only source OR destination can be specified");
                    if (pair->source.empty() && pair->dest.empty())
                        throw Err_Custom("Need to specify source OR destination");

                    DBSingleton::instance().getDBObjectInstance()->setBandwidthLimit(
                        pair->source, pair->dest, pair->limit);

                    if (pair->limit >= 0)
                        {
                            FTS3_COMMON_LOGGER_NEWLOG (INFO)
                                    << "User: "
                                    << dn
                                    << " had set the maximum bandwidth of "
                                    << pair->source << pair->dest
                                    << " to "
                                    << pair->limit << "MB/s"
                                    << commit;
                        }
                    else
                        {
                            FTS3_COMMON_LOGGER_NEWLOG (INFO)
                                    << "User: "
                                    << dn
                                    << " had reset the maximum bandwidth of "
                                    << pair->source << pair->dest
                                    << commit;
                        }
                    // prepare the command for audit
                    stringstream cmd;

                    cmd << dn;
                    cmd << " had set the maximum bandwidth of ";
                    cmd << pair->source << pair->dest;
                    cmd << " to ";
                    cmd << pair->limit << "MB/s";
                    DBSingleton::instance().getDBObjectInstance()->auditConfiguration(dn, cmd.str(), "max-bandwidth");
                }
        }
    catch(Err& ex)
        {

            FTS3_COMMON_LOGGER_NEWLOG (ERR) << "An exception has been caught: " << ex.what() << commit;
            soap_receiver_fault(ctx, ex.what(), "InvalidConfigurationException");

            return SOAP_FAULT;
        }
    catch (...)
        {
            FTS3_COMMON_LOGGER_NEWLOG (ERR) << "An exception has been thrown, setBandwidthLimit cannot be set"  << commit;
            return SOAP_FAULT;
        }

    return SOAP_OK;
}

int fts3::implcfg__getBandwidthLimit(soap* ctx, fts3::implcfg__getBandwidthLimitResponse& resp)
{
    try
        {
            resp.limit = DBSingleton::instance().getDBObjectInstance()->getBandwidthLimit();
        }
    catch(Err& ex)
        {

            FTS3_COMMON_LOGGER_NEWLOG (ERR) << "An exception has been caught: " << ex.what() << commit;
            soap_receiver_fault(ctx, ex.what(), "InvalidConfigurationException");

            return SOAP_FAULT;
        }
    catch (...)
        {
            FTS3_COMMON_LOGGER_NEWLOG (ERR) << "An exception has been thrown, getBandwidthLimit cannot be set"  << commit;
            return SOAP_FAULT;
        }

    return SOAP_OK;
}

int fts3::implcfg__setSeProtocol(soap* ctx, string protocol, string se, string state, implcfg__setSeProtocolResponse &resp)
{

    try
        {
            if (state != "on" && state != "off") throw Err_Custom("the protocol may be either set to 'on' or 'off'");

            // Authorise operation
            AuthorizationManager::getInstance().authorize(ctx, AuthorizationManager::CONFIG, AuthorizationManager::dummy);

            DBSingleton::instance().getDBObjectInstance()->setSeProtocol(protocol, se, state);

            // get user DN
            CGsiAdapter cgsi(ctx);
            string dn = cgsi.getClientDn();

            // audit the operation
            string cmd = "fts3-config-set --protocol " + protocol + " " + se + " " + state;
            DBSingleton::instance().getDBObjectInstance()->auditConfiguration(dn, cmd, "protocol");
        }
    catch(Err& ex)
        {
            FTS3_COMMON_LOGGER_NEWLOG (ERR) << "An exception has been caught: " << ex.what() << commit;
            soap_receiver_fault(ctx, ex.what(), "InvalidConfigurationnException");
            return SOAP_FAULT;
        }
    catch (...)
        {
            FTS3_COMMON_LOGGER_NEWLOG (ERR) << "An exception has been thrown"  << commit;
            return SOAP_FAULT;
        }

    return SOAP_OK;
}

int fts3::implcfg__maxSrcSeActive(soap* ctx, string se, int active, implcfg__maxSrcSeActiveResponse& resp)
{
    try
        {
            // Authorise
            AuthorizationManager::getInstance().authorize(
                ctx,
                AuthorizationManager::CONFIG,
                AuthorizationManager::dummy
            );

            CGsiAdapter cgsi(ctx);
            string vo = cgsi.getClientVo();
            string dn = cgsi.getClientDn();

            DBSingleton::instance().getDBObjectInstance()->setSourceMaxActive(se, active);

            // prepare the command for audit
            stringstream cmd;
            cmd << dn;
            cmd << " had set the maximum number of active for source SE: ";
            cmd << se;
            cmd << " to ";
            cmd << active;
            DBSingleton::instance().getDBObjectInstance()->auditConfiguration(dn, cmd.str(), "max-se-source-active");

        }
    catch(Err& ex)
        {

            FTS3_COMMON_LOGGER_NEWLOG (ERR) << "An exception has been caught: " << ex.what() << commit;
            soap_receiver_fault(ctx, ex.what(), "InvalidConfigurationException");

            return SOAP_FAULT;
        }
    catch (...)
        {
            FTS3_COMMON_LOGGER_NEWLOG (ERR) << "An exception has been thrown, the maxSrcSeActive failed"  << commit;
            return SOAP_FAULT;
        }

    return SOAP_OK;
}

int fts3::implcfg__maxDstSeActive(soap* ctx, string se, int active, implcfg__maxDstSeActiveResponse& resp)
{
    try
        {
            // Authorise
            AuthorizationManager::getInstance().authorize(
                ctx,
                AuthorizationManager::CONFIG,
                AuthorizationManager::dummy
            );

            CGsiAdapter cgsi(ctx);
            string vo = cgsi.getClientVo();
            string dn = cgsi.getClientDn();

            DBSingleton::instance().getDBObjectInstance()->setDestMaxActive(se, active);

            // prepare the command for audit
            stringstream cmd;
            cmd << dn;
            cmd << " had set the maximum number of active for destination SE: ";
            cmd << se;
            cmd << " to ";
            cmd << active;
            DBSingleton::instance().getDBObjectInstance()->auditConfiguration(dn, cmd.str(), "max-se-dest-active");
        }
    catch(Err& ex)
        {

            FTS3_COMMON_LOGGER_NEWLOG (ERR) << "An exception has been caught: " << ex.what() << commit;
            soap_receiver_fault(ctx, ex.what(), "InvalidConfigurationException");

            return SOAP_FAULT;
        }
    catch (...)
        {
            FTS3_COMMON_LOGGER_NEWLOG (ERR) << "An exception has been thrown, the maxDstSeActive failed"  << commit;
            return SOAP_FAULT;
        }

    return SOAP_OK;
}

int fts3::implcfg__fixActivePerPair(soap* ctx, string source, string destination, int active, implcfg__fixActivePerPairResponse& resp)
{
    try
        {
            AuthorizationManager::getInstance().authorize(
                ctx,
                AuthorizationManager::CONFIG,
                AuthorizationManager::dummy
            );

            CGsiAdapter cgsi(ctx);
            string vo = cgsi.getClientVo();
            string dn = cgsi.getClientDn();

            DBSingleton::instance().getDBObjectInstance()->setFixActive(source, destination, active);

            // prepare the command for audit
            stringstream cmd;
            cmd << dn;
            cmd << " had set the fixed number of active between ";
            cmd << source;
            cmd << " and ";
            cmd << destination;
            cmd << " to ";
            cmd << active;
            DBSingleton::instance().getDBObjectInstance()->auditConfiguration(dn, cmd.str(), "fix-active-per-pair");
        }
    catch(Err& ex)
        {

            FTS3_COMMON_LOGGER_NEWLOG (ERR) << "An exception has been caught: " << ex.what() << commit;
            soap_receiver_fault(ctx, ex.what(), "InvalidConfigurationException");

            return SOAP_FAULT;
        }
    catch (...)
        {
            FTS3_COMMON_LOGGER_NEWLOG (ERR) << "An exception has been thrown, the fixActivePerPair failed"  << commit;
            return SOAP_FAULT;
        }
    return SOAP_OK;
}

int fts3::implcfg__setSecPerMb(soap* ctx, int secPerMb, implcfg__setSecPerMbResponse& resp)
{
    try
        {
            // Authorise
            AuthorizationManager::getInstance().authorize(
                ctx,
                AuthorizationManager::CONFIG,
                AuthorizationManager::dummy
            );

            CGsiAdapter cgsi(ctx);
            string vo = cgsi.getClientVo();
            string dn = cgsi.getClientDn();

            DBSingleton::instance().getDBObjectInstance()->setSecPerMb(secPerMb);

            // prepare the command for audit
            stringstream cmd;
            cmd << dn;
            cmd << " had set the seconds per MB to " << secPerMb;
            DBSingleton::instance().getDBObjectInstance()->auditConfiguration(dn, cmd.str(), "sec-per-mb");
        }
    catch(Err& ex)
        {

            FTS3_COMMON_LOGGER_NEWLOG (ERR) << "An exception has been caught: " << ex.what() << commit;
            soap_receiver_fault(ctx, ex.what(), "InvalidConfigurationException");

            return SOAP_FAULT;
        }
    catch (...)
        {
            FTS3_COMMON_LOGGER_NEWLOG (ERR) << "An exception has been thrown, the setSecPerMb failed"  << commit;
            return SOAP_FAULT;
        }

    return SOAP_OK;
}

int fts3::implcfg__setGlobalTimeout(soap* ctx, int timeout, implcfg__setGlobalTimeoutResponse& resp)
{
    try
        {
            // Authorise
            AuthorizationManager::getInstance().authorize(
                ctx,
                AuthorizationManager::CONFIG,
                AuthorizationManager::dummy
            );

            CGsiAdapter cgsi(ctx);
            string vo = cgsi.getClientVo();
            string dn = cgsi.getClientDn();

            DBSingleton::instance().getDBObjectInstance()->setGlobalTimeout(timeout);

            // prepare the command for audit
            stringstream cmd;
            cmd << dn;
            cmd << " had set the global timeout to " << timeout;
            DBSingleton::instance().getDBObjectInstance()->auditConfiguration(dn, cmd.str(), "global-timeout");
        }
    catch(Err& ex)
        {

            FTS3_COMMON_LOGGER_NEWLOG (ERR) << "An exception has been caught: " << ex.what() << commit;
            soap_receiver_fault(ctx, ex.what(), "InvalidConfigurationException");

            return SOAP_FAULT;
        }
    catch (...)
        {
            FTS3_COMMON_LOGGER_NEWLOG (ERR) << "An exception has been thrown, the setGlobalTimeout failed"  << commit;
            return SOAP_FAULT;
        }

    return SOAP_OK;
}

int fts3::implcfg__setS3Ceredential(soap* ctx, std::string accessKey, std::string secretKey, std::string vo, std::string storage, implcfg__setS3CeredentialResponse& resp)
{
    try
        {
            // only Root is allowed to set S3 credentials
            CGsiAdapter cgsi(ctx);
            if (!cgsi.isRoot()) throw Err_Custom("Only root is allowed to set S3 credentials!");

            // make sure the host name is upper case
            boost::to_upper(storage);
            DBSingleton::instance().getDBObjectInstance()->setCloudStorageCredential(
                cgsi.getClientDn(), vo, storage, accessKey, secretKey
            );
        }
    catch(Err& ex)
        {

            FTS3_COMMON_LOGGER_NEWLOG (ERR) << "An exception has been caught: " << ex.what() << commit;
            soap_receiver_fault(ctx, ex.what(), "InvalidConfigurationException");

            return SOAP_FAULT;
        }
    catch (...)
        {
            FTS3_COMMON_LOGGER_NEWLOG (ERR) << "An exception has been thrown, the setGlobalTimeout failed"  << commit;
            return SOAP_FAULT;
        }

    return SOAP_OK;
}

int fts3::implcfg__setDropboxCeredential(soap* ctx, std::string appKey, std::string appSecret, std::string apiUrl, implcfg__setDropboxCeredentialResponse& resp)
{
    try
        {
            // only Root is allowed to set S3 credentials
            CGsiAdapter cgsi(ctx);
            if (!cgsi.isRoot()) throw Err_Custom("Only root is allowed to set S3 credentials!");

            DBSingleton::instance().getDBObjectInstance()->setCloudStorage("dropbox", appKey, appSecret, apiUrl);

        }
    catch(Err& ex)
        {

            FTS3_COMMON_LOGGER_NEWLOG (ERR) << "An exception has been caught: " << ex.what() << commit;
            soap_receiver_fault(ctx, ex.what(), "InvalidConfigurationException");

            return SOAP_FAULT;
        }
    catch (...)
        {
            FTS3_COMMON_LOGGER_NEWLOG (ERR) << "An exception has been thrown, the setGlobalTimeout failed"  << commit;
            return SOAP_FAULT;
        }

    return SOAP_OK;
}

int fts3::impltns__debugLevelSet(struct soap* soap, string source, string destination, int level, struct impltns__debugLevelSetResponse &_resp)
{
//  FTS3_COMMON_LOGGER_NEWLOG (INFO) << "Handling 'debugLevelSet' request" << commit;
    try
        {
            CGsiAdapter cgsi(soap);
            string dn = cgsi.getClientDn();

            FTS3_COMMON_LOGGER_NEWLOG (INFO) << "DN: " << dn << " is setting debug level to " << level
                    << "for source: " << source << " and destination: " << destination << commit;

            AuthorizationManager::getInstance().authorize(soap, AuthorizationManager::CONFIG, AuthorizationManager::dummy);

            if (!source.empty())
                DBSingleton::instance().getDBObjectInstance()->setDebugLevel(source, std::string(), level);

            if (!destination.empty())
                DBSingleton::instance().getDBObjectInstance()->setDebugLevel(std::string(), destination, level);

            string cmd = "fts3-debug-set ";
            if (!source.empty()) cmd += " --source " + source;
            if (!destination.empty()) cmd += " --destination " + destination;
            cmd += " " + boost::lexical_cast<std::string>(level);
            DBSingleton::instance().getDBObjectInstance()->auditConfiguration(dn, cmd, "debug");
        }
    catch(Err& ex)
        {
            FTS3_COMMON_LOGGER_NEWLOG (ERR) << "An exception has been caught: " << ex.what() << commit;
            soap_receiver_fault(soap, ex.what(), "TransferException");
            return SOAP_FAULT;
        }
    catch (...)
        {
            FTS3_COMMON_LOGGER_NEWLOG (ERR) << "An exception has been thrown"  << commit;
            return SOAP_FAULT;
        }
    return SOAP_OK;
}

int fts3::impltns__blacklistSe(struct soap* ctx, std::string name, std::string vo, std::string status, int timeout, bool blk, struct impltns__blacklistSeResponse &resp)
{

    try
        {
            AuthorizationManager::getInstance().authorize(ctx, AuthorizationManager::CONFIG, AuthorizationManager::dummy);

            Blacklister blacklister (ctx, name, vo, status, timeout, blk);
            blacklister.executeRequest();
        }
    catch(Err& ex)
        {
            FTS3_COMMON_LOGGER_NEWLOG (ERR) << "An exception has been caught: " << ex.what() << commit;
            soap_receiver_fault(ctx, ex.what(), "TransferException");
            return SOAP_FAULT;
        }
    catch (...)
        {
            FTS3_COMMON_LOGGER_NEWLOG (ERR) << "An exception has been thrown "  << commit;
            return SOAP_FAULT;
        }

    return SOAP_OK;
}

int fts3::impltns__blacklistDn(soap* ctx, string subject, bool blk, string status, int timeout, impltns__blacklistDnResponse &resp)
{

    try
        {

            AuthorizationManager::getInstance().authorize(ctx, AuthorizationManager::CONFIG, AuthorizationManager::dummy);

            Blacklister blacklister(ctx, subject, status, timeout, blk);
            blacklister.executeRequest();

        }
    catch(Err& ex)
        {
            FTS3_COMMON_LOGGER_NEWLOG (ERR) << "An exception has been caught: " << ex.what() << commit;
            soap_receiver_fault(ctx, ex.what(), "TransferException");
            return SOAP_FAULT;
        }
    catch (...)
        {
            FTS3_COMMON_LOGGER_NEWLOG (ERR) << "An exception has been thrown"  << commit;
            return SOAP_FAULT;
        }

    return SOAP_OK;
}

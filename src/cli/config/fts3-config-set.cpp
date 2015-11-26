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


#include "ServiceAdapterFallbackFacade.h"
#include "ui/SetCfgCli.h"
#include "exception/cli_exception.h"

#include <string>
#include <vector>
#include <memory>
#include "../../common/Exceptions.h"

using namespace fts3::cli;
using namespace fts3::common;

/**
 * This is the entry point for the fts3-config-set command line tool.
 */
int main(int ac, char* av[])
{
    try
        {
            SetCfgCli cli;
            // create and initialize the command line utility
            cli.parse(ac, av);
            if (cli.printHelp()) return 0;
            cli.validate();

            // validate command line options, and return service context
            ServiceAdapterFallbackFacade ctx(cli.getService(), cli.capath(), cli.proxy());
            cli.printApiDetails(ctx);

            boost::optional<std::tuple<std::string, std::string, std::string, std::string> > s3 = cli.s3();
            if (s3)
                {
                    ctx.setS3Credential(std::get<0>(*s3), std::get<1>(*s3), std::get<2>(*s3), std::get<3>(*s3));
                    std::cout << "Done" << std::endl;
                }

            boost::optional<std::tuple<std::string, std::string, std::string> > dropbox = cli.dropbox();
            if (dropbox)
                {
                    ctx.setDropboxCredential(std::get<0>(*dropbox), std::get<1>(*dropbox), std::get<2>(*dropbox));
                    std::cout << "Done" << std::endl;
                }

            boost::optional<std::tuple<std::string, std::string, std::string>> protocol = cli.getProtocol();
            if (protocol.is_initialized())
                {
                    std::string prot = std::get<0>(*protocol);
                    std::string se = std::get<1>(*protocol);
                    std::string state = std::get<2>(*protocol);
                    ctx.setSeProtocol(prot, se, state);

                    std::cout << "Done, just applied: " << prot << " " << se << " " << state << std::endl;
                    return 0;
                }

            boost::optional< std::pair<std::string, int> > maxActivePerSe = cli.getMaxSrcSeActive();
            if (maxActivePerSe.is_initialized())
                {
                    ctx.setMaxSrcSeActive(maxActivePerSe.get().first, maxActivePerSe.get().second);
                    std::cout << "Done, just applied: " << maxActivePerSe.get().first << " " << maxActivePerSe.get().second  << std::endl;
                }

            maxActivePerSe = cli.getMaxDstSeActive();
            if (maxActivePerSe.is_initialized())
                {
                    ctx.setMaxDstSeActive(maxActivePerSe.get().first, maxActivePerSe.get().second);
                    std::cout << "Done, just applied: " << maxActivePerSe.get().first << " " << maxActivePerSe.get().second  << std::endl;
                }

            boost::optional<bool> drain = cli.drain();
            if (drain.is_initialized())
                {
                    ctx.doDrain(drain.get());
                    std::cout << "Done" << std::endl;
                }

            boost::optional<bool> showUserDn = cli.showUserDn();
            if (showUserDn)
                {
                    ctx.showUserDn(*showUserDn);
                    std::cout << "Done" << std::endl;
                }

            boost::optional< std::pair<std::string, int> > retry = cli.retry();
            if (retry.is_initialized())
                {
                    ctx.retrySet(retry->first, retry->second);
                    std::cout << "Done" << std::endl;
                }

            boost::optional<int> mode = cli.optimizer_mode();
            if (mode.is_initialized())
                {
                    ctx.optimizerModeSet(*mode);
                    std::cout << "Done" << std::endl;
                }

            boost::optional<int> secPerMb = cli.getSecPerMb();
            if (secPerMb.is_initialized())
                {
                    ctx.setSecPerMb(*secPerMb);
                    std::cout << "Done" << std::endl;
                }

            boost::optional<int> globalTimeout = cli.getGlobalTimeout();
            if (globalTimeout.is_initialized())
                {
                    ctx.setGlobalTimeout(*globalTimeout);
                    std::cout << "Done" << std::endl;
                }

            boost::tuple<boost::optional<int>, boost::optional<int> > globalLimits = cli.getGlobalLimits();
            if (globalLimits.get<0>().is_initialized() || globalLimits.get<1>().is_initialized())
                {
                    ctx.setGlobalLimits(globalLimits.get<0>(), globalLimits.get<1>());
                    std::cout << "Done" << std::endl;
                }

            boost::optional<unsigned> queueTimeout = cli.queueTimeout();
            if (queueTimeout.is_initialized())
                {
                    ctx.queueTimeoutSet(*queueTimeout);
                    std::cout << "Done" << std::endl;
                }

            boost::optional<std::tuple<std::string, int, std::string>> bring_online = cli.getBringOnline();
            if (bring_online)
                {
                    ctx.setMaxOpt(*bring_online, "staging");
                    // if bring online was used normal config was not!
                    std::cout << "Done" << std::endl;
                    return 0;
                }

            boost::optional<std::tuple<std::string, int, std::string>> delete_opt = cli.getDelete();
            if (delete_opt)
                {
                    ctx.setMaxOpt(*delete_opt, "delete");
                    // if delete was used normal config was not!
                    std::cout << "Done" << std::endl;
                    return 0;
                }

            boost::optional<std::tuple<std::string, std::string, int> > bandwidth_limitation = cli.getBandwidthLimitation();
            if (bandwidth_limitation)
                {
                    ctx.setBandwidthLimit(std::get<0>(*bandwidth_limitation),
                                          std::get<1>(*bandwidth_limitation),
                                          std::get<2>(*bandwidth_limitation));
                    std::cout << "Done" << std::endl;
                    return 0;
                }

            boost::optional<std::tuple<std::string, std::string, int> > active_fixed = cli.getActiveFixed();
            if (active_fixed)
                {
                    ctx.setFixActivePerPair(std::get<0>(*active_fixed),
                                            std::get<1>(*active_fixed),
                                            std::get<2>(*active_fixed));
                    std::cout << "Done" << std::endl;
                    return 0;
                }

            boost::optional<std::tuple<std::string, std::string> > addAuthz = cli.getAddAuthorization();
            if (addAuthz)
                {
                    ctx.authorize(std::get<0>(*addAuthz), std::get<1>(*addAuthz));
                    std::cout << "Done" << std::endl;
                    return 0;
                }

            boost::optional<std::tuple<std::string, std::string> > revokeAuthz = cli.getRevokeAuthorization();
            if (revokeAuthz)
                {
                    ctx.revoke(std::get<0>(*revokeAuthz), std::get<1>(*revokeAuthz));
                    std::cout << "Done" << std::endl;
                    return 0;
                }

            std::vector<std::string> cfg = cli.getConfigurations();
            if (cfg.empty()) return 0;

            ctx.setConfiguration(cfg);

            std::cout << "Done" << std::endl;


        }
    catch(cli_exception const & ex)
        {
            MsgPrinter::instance().print(ex);
            return 1;
        }
    catch(std::exception& ex)
        {
            MsgPrinter::instance().print(ex);
            return 1;
        }
    catch(...)
        {
            MsgPrinter::instance().print("error", "exception of unknown type!");
            return 1;
        }

    return 0;
}

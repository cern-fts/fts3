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

#include "fts3.nsmap"
#include "config/serverconfig.h"
#include <cgsi_plugin.h>
#include <signal.h>
#include <sys/socket.h>
#include <fstream>
#include "GsoapAcceptor.h"

#include "GsoapRequestHandler.h"

bool  stopThreads;


using namespace fts3::config;
using namespace fts3::common;
using namespace fts3::server;


GSoapAcceptor::GSoapAcceptor(const unsigned int port, const std::string& ip)
{

    bool keepAlive = theServerConfig().get<std::string>("HttpKeepAlive")=="true" ? true : false;
    if (keepAlive)
        {
            ctx = soap_new2(SOAP_IO_KEEPALIVE, SOAP_IO_KEEPALIVE);

            ctx->bind_flags |= SO_REUSEADDR;
            ctx->accept_timeout = 180;
            ctx->recv_timeout = 110; // Timeout after 2 minutes stall on recv
            ctx->send_timeout = 110; // Timeout after 2 minute stall on send

            soap_cgsi_init(ctx,  CGSI_OPT_KEEP_ALIVE  | CGSI_OPT_SERVER | CGSI_OPT_SSL_COMPATIBLE | CGSI_OPT_DISABLE_MAPPING);// | CGSI_OPT_DISABLE_NAME_CHECK);
            soap_set_namespaces(ctx, fts3_namespaces);


            SOAP_SOCKET sock = soap_bind(ctx, ip.c_str(), static_cast<int>(port), 300);
            if (sock >= 0)
                {
                    ctx->socket_flags |= MSG_NOSIGNAL; // use this, prevent sigpipe
                    FTS3_COMMON_LOGGER_NEWLOG (INFO) << "Soap service " << sock << " IP:" << ip << " Port:" << port << commit;
                }
            else
                {
                    FTS3_COMMON_EXCEPTION_THROW (Err_System ("Unable to bind socket."));
                    fclose (stderr);
                    _exit(1);
                }

        }
    else
        {
            ctx = soap_new();
            ctx->recv_timeout = 110; // Timeout after 2 minutes stall on recv
            ctx->send_timeout = 110; // Timeout after 2 minute stall on send
            ctx->accept_timeout = 180;
            ctx->bind_flags |= SO_REUSEADDR;
            soap_cgsi_init(ctx,  CGSI_OPT_SERVER | CGSI_OPT_SSL_COMPATIBLE | CGSI_OPT_DISABLE_MAPPING);// | CGSI_OPT_DISABLE_NAME_CHECK);
            soap_set_namespaces(ctx, fts3_namespaces);


            SOAP_SOCKET sock = soap_bind(ctx, ip.c_str(), static_cast<int>(port), 300);

            if (sock >= 0)
                {
                    ctx->socket_flags |= MSG_NOSIGNAL; // use this, prevent sigpipe
                    FTS3_COMMON_LOGGER_NEWLOG (INFO) << "Soap service " << sock << " IP:" << ip << " Port:" << port << commit;
                }
            else
                {
                    FTS3_COMMON_EXCEPTION_THROW (Err_System ("Unable to bind socket."));
                    fclose (stderr);
                    _exit(1);
                }
        }
}

GSoapAcceptor::~GSoapAcceptor()
{
    soap* tmp=NULL;
    while (!recycle.empty())
        {
            tmp = recycle.front();
            if(tmp)
                {
                    recycle.pop();
                    soap_clr_omode(tmp, SOAP_IO_KEEPALIVE);
                    shutdown(tmp->socket,2);
                    shutdown(tmp->master,2);
                    soap_destroy(tmp);
                    soap_end(tmp);
                    soap_done(tmp);
                    soap_free(tmp);
                }
        }
    if(ctx)
        {
            soap_clr_omode(ctx, SOAP_IO_KEEPALIVE);
            shutdown(ctx->master,2);
            shutdown(ctx->socket,2);
            soap_destroy(ctx);
            soap_end(ctx);
            soap_done(ctx);
            soap_free(ctx);
        }
}

std::unique_ptr<GSoapRequestHandler> GSoapAcceptor::accept()
{
    SOAP_SOCKET sock = soap_accept(ctx);
    std::unique_ptr<GSoapRequestHandler> handler;

    if (sock >= 0)
        {
            handler.reset (
                new GSoapRequestHandler(*this)
            );

            char ipbuffer [512] = {0};
            sprintf(ipbuffer, "accepted connection from host=%s, socket=%d", ctx->host, sock);
            FTS3_COMMON_LOGGER_NEWLOG (INFO) << ipbuffer << commit;
        }
    else
        {
            FTS3_COMMON_EXCEPTION_LOGERROR (Err_System ("Unable to accept connection request."));
        }

    return handler;
}

soap* GSoapAcceptor::getSoapContext()
{
    boost::recursive_mutex::scoped_lock lock(_mutex);
    if (!recycle.empty())
        {
            soap* ctx = recycle.front();
            recycle.pop();
            if(ctx)
                {
                    ctx->socket = this->ctx->socket;
                    return ctx;
                }
        }

    soap* temp = soap_copy(ctx);
    temp->bind_flags |= SO_REUSEADDR;
    temp->accept_timeout = 180;
    temp->recv_timeout = 110; // Timeout after 2 minutes stall on recv
    temp->send_timeout = 110; // Timeout after 2 minute stall on send
    temp->socket_flags |= MSG_NOSIGNAL; // use this, prevent sigpipe

    return temp;
}

void GSoapAcceptor::recycleSoapContext(soap* ctx)
{
    if(stopThreads)
        return;

    boost::recursive_mutex::scoped_lock lock(_mutex);

    if(ctx)
        {
            soap_destroy(ctx);
            soap_end(ctx);

            ctx->bind_flags |= SO_REUSEADDR;
            ctx->accept_timeout = 180;
            ctx->recv_timeout = 110; // Timeout after 2 minutes stall on recv
            ctx->send_timeout = 110; // Timeout after 2 minute stall on send
            ctx->socket_flags |= MSG_NOSIGNAL; // use this, prevent sigpipe
            recycle.push(ctx);
        }
}


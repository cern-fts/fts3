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
#include <cgsi_plugin.h>
#include <signal.h>
#include <sys/socket.h>
#include <fstream>

#include "common/Logger.h"
#include "GsoapAcceptor.h"

#include "config/ServerConfig.h"
#include "GsoapRequestHandler.h"


using namespace fts3::config;
using namespace fts3::common;
using namespace fts3::server;


GSoapAcceptor::GSoapAcceptor(const unsigned int port, const std::string& ip)
{
    bool keepAlive = ServerConfig::instance().get<bool>("HttpKeepAlive");
    int cgsi_options = CGSI_OPT_SERVER | CGSI_OPT_SSL_COMPATIBLE | CGSI_OPT_DISABLE_MAPPING;

    if (keepAlive) {
        ctx = soap_new2(SOAP_IO_KEEPALIVE, SOAP_IO_KEEPALIVE);
        cgsi_options |= CGSI_OPT_KEEP_ALIVE;
    }
    else {
        ctx = soap_new();
    }

    soap_cgsi_init(ctx,  cgsi_options);
    soap_set_namespaces(ctx, fts3_namespaces);

    ctx->bind_flags |= SO_REUSEADDR;
    ctx->accept_timeout = 10;
    ctx->recv_timeout = 120;
    ctx->send_timeout = 120;

    SOAP_SOCKET sock = soap_bind(ctx, ip.c_str(), static_cast<int>(port), 300);

    if (sock >= 0) {
        ctx->socket_flags |= MSG_NOSIGNAL; // use this, prevent sigpipe
        FTS3_COMMON_LOGGER_NEWLOG (INFO) << "Soap service " << sock << " IP:" << ip << " Port:" << port << commit;
    }
    else {
        throw SystemError("Unable to bind socket.");
    }
}

GSoapAcceptor::~GSoapAcceptor()
{
    soap *tmp = NULL;
    while (!recycle.empty()) {
        tmp = recycle.front();
        if (tmp) {
            recycle.pop();
            soap_clr_omode(tmp, SOAP_IO_KEEPALIVE);
            shutdown(tmp->socket, 2);
            shutdown(tmp->master, 2);
            soap_destroy(tmp);
            soap_end(tmp);
            soap_done(tmp);
            soap_free(tmp);
        }
    }
    if (ctx) {
        soap_clr_omode(ctx, SOAP_IO_KEEPALIVE);
        shutdown(ctx->master, 2);
        shutdown(ctx->socket, 2);
        soap_destroy(ctx);
        soap_end(ctx);
        soap_done(ctx);
        soap_free(ctx);
    }
}


std::unique_ptr<GSoapRequestHandler> GSoapAcceptor::accept()
{
    SOAP_SOCKET socket;

    do {
        socket = soap_accept(ctx);
    } while (socket < 0 && !boost::this_thread::interruption_requested());

    if (socket >= 0) {
        FTS3_COMMON_LOGGER_NEWLOG (INFO)
            << "Accepted connection from host=" << ctx->host << ", socket=" << socket
            << commit;
        return std::unique_ptr<GSoapRequestHandler>(new GSoapRequestHandler(*this));
    }
    else if (boost::this_thread::interruption_requested()) {
        throw boost::thread_interrupted();
    }

    throw SystemError("Unable to accept connection request.");
}


soap *GSoapAcceptor::getSoapContext()
{
    boost::recursive_mutex::scoped_lock lock(_mutex);
    if (!recycle.empty()) {
        soap *ctx = recycle.front();
        recycle.pop();
        if (ctx) {
            ctx->socket = this->ctx->socket;
            return ctx;
        }
    }

    return soap_copy(ctx);
}


void GSoapAcceptor::recycleSoapContext(soap *ctx)
{
    if (boost::this_thread::interruption_requested()) {
        return;
    }

    boost::recursive_mutex::scoped_lock lock(_mutex);

    if (ctx) {
        soap_destroy(ctx);
        soap_end(ctx);
        recycle.push(ctx);
    }
}

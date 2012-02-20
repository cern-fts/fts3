/* Copyright @ Members of the EMI Collaboration, 2010.
See www.eu-emi.eu for details on the copyright holders.

Licensed under the Apache License, Version 2.0 (the "License"); 
you may not use this file except in compliance with the License. 
You may obtain a copy of the License at 

    http://www.apache.org/licenses/LICENSE-2.0 

Unless required by applicable law or agreed to in writing, software 
distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
See the License for the specific language governing permissions and 
limitations under the License. */

/** \file server.cpp Implementation of FTS3 server logic. */

#include "common/error.h"
//#include "ThreadPool.h"
#include "config/serverconfig.h"
//#include "dispatcher.h"
//#include "CoralDataAcceptor.h"
#include "server.h"

FTS3_SERVER_NAMESPACE_START

using namespace FTS3_COMMON_NAMESPACE;

/* -------------------------------------------------------------------------- */

void Server::start()
{
#if 0
	typedef Dispatcher<CORALMESSAGING_NAMESPACE_NAME::Tcp::TcpSocketSelector> DispatcherType;
	DispatcherType portDisp("for incoming TCP connections");
	Pointer<ICoralIOHandler>::Shared dataChannel = CORALMESSAGING_NAMESPACE_NAME::Tcp::TcpSocketFactory::CreateListeningSocket(port, ip);
	Pointer<IEventHandler>::Shared cdacc(new CoralDataAcceptor());
	portDisp.registerService(dataChannel, cdacc);
	portDisp.start_p();
	ThreadPool::ThreadPool::instance().wait();
#endif
}

/* -------------------------------------------------------------------------- */

void Server::stop()
{
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "FTS3 server stopping...";
    //ThreadPool::ThreadPool::instance().stop();
    theLogger() << commit;
}

FTS3_SERVER_NAMESPACE_END


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
 *
 * LogFileStreamer.cpp
 *
 *  Created on: Oct 12, 2012
 *      Author: Michał Simon
 */

#include "LogFileStreamer.h"
#include "common/TarArchiver.h"

#include "error.h"
#include <iostream>
#include <vector>

using namespace fts3::ws;


void* LogFileStreamer::readOpen(soap* ctx, void* handle, const char* id, const char* type, const char* description)
{
    return handle;
}

size_t LogFileStreamer::read(soap* ctx, void* handle, char* buff, size_t len)
{

    fstream* file = (fstream*) handle;
    if (file->eof()) return 0;

    file->read(buff, len);
    return file->gcount();
}

void LogFileStreamer::readClose(soap* ctx, void* handle)
{
    fstream* file = (fstream*) handle;
    if (file)
        {
            file->close();
            delete file;
        }
}

void* LogFileStreamer::writeOpen(soap* ctx, void* handle, const char *id, const char *type, const char *description, soap_mime_encoding encoding)
{

    OutputHandler* oh = (OutputHandler*)ctx->user;
    string out = oh->getLogName();
    if (oh->empty()) delete oh;

    fstream* file = new fstream(out.c_str(), ios::out | ios::trunc);
    return (void*) file;
}

int LogFileStreamer::write(soap* ctx, void *handle, const char *buff, size_t len)
{

    fstream* file = (fstream*) handle;
    file->write(buff, len);
    return SOAP_OK;
}

void LogFileStreamer::writeClose(soap* ctx, void *handle)
{
    fstream* file = (fstream*) handle;
    if (file)
        {
            file->flush();
            file->close();
            delete file;
        }
}

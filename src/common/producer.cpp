/*
 *	Copyright notice:
 *	Copyright © Members of the EMI Collaboration, 2010.
 *
 *	See www.eu-emi.eu for details on the copyright holders
 *
 *	Licensed under the Apache License, Version 2.0 (the "License");
 *	you may not use this file except in compliance with the License.
 *	You may obtain a copy of the License at
 *
 *		http://www.apache.org/licenses/LICENSE-2.0
 *
 *	Unless required by applicable law or agreed to in writing, software
 *	distributed under the License is distributed on an "AS IS" BASIS,
 *	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *	See the License for the specific language governing permissions and
 *	limitations under the License.
 *
 */

#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <vector>
#include <string>
#include <iostream>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cstdlib>
#include <string>
#include <sstream>
#include <iostream>
#include <boost/lexical_cast.hpp>
#include "producer_consumer_common.h"
#include "uuid_generator.h"
using namespace std;



void getUniqueTempFileName(const std::string& basename,
                           std::string& tempname)
{
    std::string uuidGen = UuidGenerator::generateUUID();
    time_t tmCurrent = time(NULL);
    std::stringstream strmName;
    strmName << basename << uuidGen << "_" << tmCurrent;
    tempname = strmName.str();
}

std::string getNewMessageFile(const char* BASE_DIR)
{
    std::string basename(BASE_DIR);
    std::string tempname;
    getUniqueTempFileName(basename, tempname);
    return tempname;
}

int writeMessage(const void* buffer, size_t bufsize, const char* BASE_DIR, const std::string & extension)
{
    std::string tempname = getNewMessageFile(BASE_DIR);
    if(tempname.length() <= 0)
        return -1;
    // Open
    FILE* fp = NULL;
    fp = fopen(tempname.c_str(), "w");
    if (fp == NULL)
        return errno;

    // Try to write twice
    size_t writeBytes = fwrite(buffer, bufsize, 1, fp);
    if (writeBytes == 0)
        writeBytes = fwrite(buffer, bufsize, 1, fp);

    // Close
    fclose(fp);

    // Rename to final name (sort of commit)
    // Try twice too
    std::string renamedFile = tempname +  extension; //"_ready or _staging or _delete";
    int r = rename(tempname.c_str(), renamedFile.c_str());
    if (r == -1)
        r = rename(tempname.c_str(), renamedFile.c_str());
    if (r == -1)
        return errno;

    return 0;
}


int runProducerMonitoring(message_monitoring &msg)
{
    return writeMessage(&msg, sizeof(message_monitoring), MONITORING_DIR, "_ready");
}


int runProducerStatus(message &msg)
{
    return writeMessage(&msg, sizeof(message), STATUS_DIR, "_ready");
}


int runProducerStall(message_updater &msg)
{
    return writeMessage(&msg, sizeof(message_updater), STALLED_DIR, "_ready");
}


int runProducerLog(message_log &msg)
{
    return writeMessage(&msg, sizeof(msg), LOG_DIR, "_ready");
}

int runProducerDeletions(struct message_bringonline &msg)
{
    return writeMessage(&msg, sizeof(msg), STATUS_DM_DIR, "_delete");
}


int runProducerStaging(struct message_bringonline &msg)
{
    return writeMessage(&msg, sizeof(msg), STATUS_DM_DIR, "_staging");
}

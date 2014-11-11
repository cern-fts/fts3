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
#include <sys/stat.h>
#include "producer_consumer_common.h"
#include "definitions.h"
#include <algorithm>
#include <ctime>
#include "logger.h"

using namespace std;


struct sort_functor_updater
{
    bool operator()(const message_updater & a, const message_updater & b) const
    {
        return a.timestamp < b.timestamp;
    }
};

struct sort_functor_status
{
    bool operator()(const message & a, const message & b) const
    {
        return a.timestamp < b.timestamp;
    }
};



boost::posix_time::time_duration::tick_type milliseconds_since_epoch()
{
    using boost::gregorian::date;
    using boost::posix_time::ptime;
    using boost::posix_time::microsec_clock;

    static ptime const epoch(date(1970, 1, 1));
    return (microsec_clock::universal_time() - epoch).total_milliseconds();
}


int getDir (string dir, vector<string> &files, const std::string& extension)
{
    DIR *dp=NULL;
    struct dirent *dirp=NULL;
    struct stat st;
    if((dp  = opendir(dir.c_str())) == NULL)
        {
            return errno;
        }

    while ((dirp = readdir(dp)) != NULL)
        {
            std::string fileName = string(dirp->d_name);
            size_t found = fileName.find(extension);
            if(found!=std::string::npos)
                {
                    std::string copyFilename = dir + fileName;
                    int stCheck = stat(copyFilename.c_str(), &st);
                    if(stCheck==0 && st.st_size > 0)
                        files.push_back(copyFilename);
                    else
                        unlink(copyFilename.c_str());
                }
        }
    closedir(dp);
    return 0;
}

int runConsumerMonitoring(std::vector<struct message_monitoring>& messages)
{
    string dir = string(MONITORING_DIR);
    vector<string> files = vector<string>();
    files.reserve(300);

    if (getDir(dir,files, "ready") != 0)
        return errno;

    for (unsigned int i = 0; i < files.size(); i++)
        {
            FILE *fp = NULL;
            struct message_monitoring msg;
            if ((fp = fopen(files[i].c_str(), "r")) != NULL)
                {
                    size_t readElements = fread(&msg, sizeof(msg), 1, fp);
                    if(readElements == 0)
                        readElements = fread(&msg, sizeof(msg), 1, fp);

                    if (readElements == 1)
                        messages.push_back(msg);
                    else
                        msg.set_error(EBADMSG);

                    unlink(files[i].c_str());
                    fclose(fp);
                    fp = NULL;
                }
            else
                {
                    msg.set_error(errno);
                }
        }
    files.clear();
    return 0;
}


int runConsumerStatus(std::vector<struct message>& messages)
{
    string dir = string(STATUS_DIR);
    vector<string> files = vector<string>();
    files.reserve(300);

    if (getDir(dir,files, "ready") != 0)
        return errno;

    for (unsigned int i = 0; i < files.size(); i++)
        {
            FILE *fp = NULL;
            struct message msg;
            if ((fp = fopen(files[i].c_str(), "r")) != NULL)
                {
                    size_t readElements = fread(&msg, sizeof(message), 1, fp);
                    if(readElements == 0)
                        readElements = fread(&msg, sizeof(message), 1, fp);

                    if (readElements == 1)
                        messages.push_back(msg);
                    else
                        msg.set_error(EBADMSG);

                    unlink(files[i].c_str());
                    fclose(fp);
                }
            else
                {
                    msg.set_error(errno);
                }
        }
    files.clear();
    std::sort(messages.begin(), messages.end(), sort_functor_status());
    return 0;
}

int runConsumerStall(std::vector<struct message_updater>& messages)
{
    string dir = string(STALLED_DIR);
    vector<string> files = vector<string>();
    files.reserve(300);

    if (getDir(dir,files, "ready") != 0)
        return errno;

    for (unsigned int i = 0; i < files.size(); i++)
        {
            FILE *fp = NULL;
            struct message_updater msg_local;
            if ((fp = fopen(files[i].c_str(), "r")) != NULL)
                {
                    size_t readElements = fread(&msg_local, sizeof(message_updater), 1, fp);
                    if(readElements == 0)
                        readElements = fread(&msg_local, sizeof(message_updater), 1, fp);

                    if (readElements == 1)
                        messages.push_back(msg_local);
                    else
                        msg_local.set_error(EBADMSG);

                    unlink(files[i].c_str());
                    fclose(fp);
                }
            else
                {
                    msg_local.set_error(errno);
                }
        }
    files.clear();
    std::sort(messages.begin(), messages.end(), sort_functor_updater());

    return 0;
}


int runConsumerLog(std::map<int, struct message_log>& messages)
{
    string dir = string(LOG_DIR);
    vector<string> files = vector<string>();
    files.reserve(300);

    if (getDir(dir,files, "ready") != 0)
        return errno;

    for (unsigned int i = 0; i < files.size(); i++)
        {
            FILE *fp = NULL;
            struct message_log msg;
            if ((fp = fopen(files[i].c_str(), "r")) != NULL)
                {
                    size_t readElements = fread(&msg, sizeof(message_log), 1, fp);
                    if(readElements == 0)
                        readElements = fread(&msg, sizeof(message_log), 1, fp);

                    if (readElements == 1)
                        messages[msg.file_id] = msg;
                    else
                        msg.set_error(EBADMSG);

                    unlink(files[i].c_str());
                    fclose(fp);
                }
            else
                {
                    msg.set_error(errno);
                }
        }
    files.clear();

    return 0;
}


int runConsumerDeletions(std::vector<struct message_bringonline>& messages)
{
    string dir = string(STATUS_DM_DIR);
    vector<string> files = vector<string>();
    files.reserve(300);

    if (getDir(dir,files, "delete") != 0)
        return errno;

    for (unsigned int i = 0; i < files.size(); i++)
        {
            FILE *fp = NULL;
            struct message_bringonline msg;
            if ((fp = fopen(files[i].c_str(), "r")) != NULL)
                {
                    size_t readElements = fread(&msg, sizeof(message_bringonline), 1, fp);
                    if(readElements == 0)
                        readElements = fread(&msg, sizeof(message_bringonline), 1, fp);

                    if (readElements == 1)
                        messages.push_back(msg);
                    else
                        msg.set_error(EBADMSG);

                    unlink(files[i].c_str());
                    fclose(fp);
                }
            else
                {
                    msg.set_error(errno);
                }
        }
    files.clear();

    return 0;
}

int runConsumerStaging(std::vector<struct message_bringonline>& messages)
{
    string dir = string(STATUS_DM_DIR);
    vector<string> files = vector<string>();
    files.reserve(300);

    if (getDir(dir,files, "staging") != 0)
        return errno;

    for (unsigned int i = 0; i < files.size(); i++)
        {
            FILE *fp = NULL;
            struct message_bringonline msg;
            if ((fp = fopen(files[i].c_str(), "r")) != NULL)
                {
                    size_t readElements = fread(&msg, sizeof(message_bringonline), 1, fp);
                    if(readElements == 0)
                        readElements = fread(&msg, sizeof(message_bringonline), 1, fp);

                    if (readElements == 1)
                        messages.push_back(msg);
                    else
                        msg.set_error(EBADMSG);

                    unlink(files[i].c_str());
                    fclose(fp);
                }
            else
                {
                    msg.set_error(errno);
                }
        }
    files.clear();

    return 0;
}



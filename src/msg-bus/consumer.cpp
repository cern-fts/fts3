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

#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <vector>
#include <string>
#include <iostream>
#include <string.h>
#include <sys/stat.h>
#include <boost/filesystem.hpp>
#include "consumer.h"


struct sort_functor_updater
{
    bool operator()(const MessageUpdater & a, const MessageUpdater & b) const
    {
        return a.timestamp < b.timestamp;
    }
};

struct sort_functor_status
{
    bool operator()(const Message & a, const Message & b) const
    {
        return a.timestamp < b.timestamp;
    }
};


Consumer::Consumer(const std::string &baseDir, unsigned limit):
    baseDir(baseDir), limit(limit)
{
}


Consumer::~Consumer()
{
}


static int getDirContent(const std::string &dir, std::vector<std::string> &files,
    const std::string &extension, unsigned limit)
{
    if (boost::filesystem::is_empty(dir)) {
        return 0;
    }

    DIR *dp = NULL;
    struct dirent *dirp = NULL;
    struct stat st;
    if ((dp = opendir(dir.c_str())) == NULL) {
        return errno;
    }

    while ((dirp = readdir(dp)) != NULL && files.size() < limit) {
        std::string fileName = std::string(dirp->d_name);
        size_t found = fileName.find(extension);
        if (found != std::string::npos) {
            std::string copyFilename = dir + "/" + fileName;
            int stCheck = stat(copyFilename.c_str(), &st);
            if (stCheck == 0 && st.st_size > 0)
                files.push_back(copyFilename);
            else
                unlink(copyFilename.c_str());
        }
    }
    closedir(dp);
    return 0;
}


int Consumer::runConsumerMonitoring(std::vector<struct MessageMonitoring> &messages)
{
    std::vector<std::string> files;
    files.reserve(300);

    if (getDirContent(baseDir + "/monitoring", files, "ready", limit) != 0) {
        return errno;
    }

    for (unsigned int i = 0; i < files.size(); i++) {
        FILE *fp = NULL;
        struct MessageMonitoring msg;
        if ((fp = fopen(files[i].c_str(), "r")) != NULL) {
            size_t readElements = fread(&msg, sizeof(msg), 1, fp);
            if (readElements == 0)
                readElements = fread(&msg, sizeof(msg), 1, fp);

            if (readElements == 1)
                messages.push_back(msg);
            else
                msg.set_error(EBADMSG);

            unlink(files[i].c_str());
            fclose(fp);
            fp = NULL;
        }
        else {
            msg.set_error(errno);
        }
    }
    files.clear();
    return 0;
}


int Consumer::runConsumerStatus(std::vector<struct Message> &messages)
{
    std::vector<std::string> files;
    files.reserve(300);

    if (getDirContent(baseDir + "/status", files, "ready", limit) != 0) {
        return errno;
    }

    for (unsigned int i = 0; i < files.size(); i++) {
        FILE *fp = NULL;
        struct Message msg;
        if ((fp = fopen(files[i].c_str(), "r")) != NULL) {
            size_t readElements = fread(&msg, sizeof(Message), 1, fp);
            if (readElements == 0)
                readElements = fread(&msg, sizeof(Message), 1, fp);

            if (readElements == 1)
                messages.push_back(msg);
            else
                msg.set_error(EBADMSG);

            unlink(files[i].c_str());
            fclose(fp);
        }
        else {
            msg.set_error(errno);
        }
    }
    files.clear();
    std::sort(messages.begin(), messages.end(), sort_functor_status());
    return 0;
}


int Consumer::runConsumerStall(std::vector<struct MessageUpdater> &messages)
{
    std::vector<std::string> files;
    files.reserve(300);

    if (getDirContent(baseDir + "/stalled", files, "ready", limit) != 0) {
        return errno;
    }

    for (unsigned int i = 0; i < files.size(); i++) {
        FILE *fp = NULL;
        struct MessageUpdater msg_local;
        if ((fp = fopen(files[i].c_str(), "r")) != NULL) {
            size_t readElements = fread(&msg_local, sizeof(MessageUpdater), 1, fp);
            if (readElements == 0)
                readElements = fread(&msg_local, sizeof(MessageUpdater), 1, fp);

            if (readElements == 1)
                messages.push_back(msg_local);
            else
                msg_local.set_error(EBADMSG);

            unlink(files[i].c_str());
            fclose(fp);
        }
        else {
            msg_local.set_error(errno);
        }
    }
    files.clear();
    std::sort(messages.begin(), messages.end(), sort_functor_updater());

    return 0;
}


int Consumer::runConsumerLog(std::map<int, struct MessageLog> &messages)
{
    std::vector<std::string> files;
    files.reserve(300);

    if (getDirContent(baseDir + "/logs", files, "ready", limit) != 0) {
        return errno;
    }

    for (unsigned int i = 0; i < files.size(); i++) {
        FILE *fp = NULL;
        struct MessageLog msg;
        if ((fp = fopen(files[i].c_str(), "r")) != NULL) {
            size_t readElements = fread(&msg, sizeof(MessageLog), 1, fp);
            if (readElements == 0)
                readElements = fread(&msg, sizeof(MessageLog), 1, fp);

            if (readElements == 1)
                messages[msg.file_id] = msg;
            else
                msg.set_error(EBADMSG);

            unlink(files[i].c_str());
            fclose(fp);
        }
        else {
            msg.set_error(errno);
        }
    }
    files.clear();

    return 0;
}


int Consumer::runConsumerDeletions(std::vector<struct MessageBringonline> &messages)
{
    std::vector<std::string> files;
    files.reserve(300);

    if (getDirContent(baseDir + "/status", files, "delete", limit) != 0)
        return errno;

    for (unsigned int i = 0; i < files.size(); i++) {
        FILE *fp = NULL;
        struct MessageBringonline msg;
        if ((fp = fopen(files[i].c_str(), "r")) != NULL) {
            size_t readElements = fread(&msg, sizeof(MessageBringonline), 1, fp);
            if (readElements == 0)
                readElements = fread(&msg, sizeof(MessageBringonline), 1, fp);

            if (readElements == 1)
                messages.push_back(msg);
            else
                msg.set_error(EBADMSG);

            unlink(files[i].c_str());
            fclose(fp);
        }
        else {
            msg.set_error(errno);
        }
    }
    files.clear();

    return 0;
}


int Consumer::runConsumerStaging(std::vector<struct MessageBringonline> &messages)
{
    std::vector<std::string> files;
    files.reserve(300);

    if (getDirContent(baseDir + "/status", files, "staging", limit) != 0) {
        return errno;
    }

    for (unsigned int i = 0; i < files.size(); i++) {
        FILE *fp = NULL;
        struct MessageBringonline msg;
        if ((fp = fopen(files[i].c_str(), "r")) != NULL) {
            size_t readElements = fread(&msg, sizeof(MessageBringonline), 1, fp);
            if (readElements == 0)
                readElements = fread(&msg, sizeof(MessageBringonline), 1, fp);

            if (readElements == 1)
                messages.push_back(msg);
            else
                msg.set_error(EBADMSG);

            unlink(files[i].c_str());
            fclose(fp);
        }
        else {
            msg.set_error(errno);
        }
    }
    files.clear();

    return 0;
}

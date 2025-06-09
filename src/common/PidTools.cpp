/*
 * Copyright (c) CERN 2016
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

#include "PidTools.h"
#include <string.h>
#include <sys/sysinfo.h>
#include <unistd.h>
#include <fstream>

#include "Logger.h"
#include "Exceptions.h"

namespace fts3 {
namespace common {

// Subset of the info in /proc/<pid>/stat
// See man proc
struct ProcStatInfo {
    pid_t pid; // %d
    char *comm; // %s
    char state; // %c
    pid_t ppid; // %d
    int pgrp; // %d
    int session; // %d
    int tty_nr; // %d
    int tpgid; // %d
    unsigned flags; // %u
    long unsigned minflt; // %lu
    long unsigned cminflt; // %lu
    long unsigned majflt; // %lu
    long unsigned cmajflt; // %lu
    long unsigned utime; // %lu
    long unsigned stime; // %lu
    long int cutime; // %ld
    long int cstime; // %ld
    long int priority; // %ld
    long int nice; // %ld
    long int num_threads; // %ld
    long int itrealvalue; // %ld
    unsigned long long starttime; // %llu
    long unsigned vsize; // %lu
    long int rss; // %ld
    long unsigned rsslim; // %lu

    ProcStatInfo() {
        memset(this, 0, sizeof(*this));
        long maxarg = sysconf(_SC_ARG_MAX);
        if (maxarg < 0) {
            maxarg = 1024;
        }
        comm = (char*)malloc(maxarg);
    }

    ~ProcStatInfo() {
        free(comm);
    }
};


static int parseProcStatFile(pid_t pid, ProcStatInfo *info)
{
    char fname[1024];
    snprintf(fname, sizeof(fname), "/proc/%d/stat", pid);
    FILE *fd = fopen(fname, "r");
    if (fd == NULL) {
        FTS3_COMMON_LOGGER_NEWLOG(DEBUG) << "Could not open " << fname << " (errno=" << errno << ")" << commit;
        return -1;
    }

    int sr = fscanf(fd,
        "%d %s %c %d %d %d %d %d %u %lu %lu %lu %lu %lu %lu %ld %ld %ld %ld %ld %ld %llu %lu %ld %lu",
        &info->pid, info->comm, &info->state, &info->ppid, &info->pgrp, &info->session, &info->tty_nr,
        &info->tpgid, &info->flags, &info->minflt, &info->cminflt, &info->majflt, &info->cmajflt,
        &info->utime, &info->stime, &info->cutime, &info->cstime, &info->priority, &info->nice,
        &info->num_threads, &info->itrealvalue, &info->starttime, &info->vsize, &info->rss, &info->rsslim);

    fclose(fd);

    if (sr < 25) {
        FTS3_COMMON_LOGGER_NEWLOG(WARNING) << "Failed to parse " << fname << commit;
        return -1;
    }
    return 0;
}


static time_t getSystemBootTime()
{
    struct sysinfo sys;
    sysinfo(&sys);
    return time(NULL) - sys.uptime;
}


uint64_t getPidStartime(pid_t pid)
{
    ProcStatInfo info;
    int ret = parseProcStatFile(pid, &info);
    if (ret < 0) {
        return 0;
    }

    // info.starttime has the start time of the process *in clock ticks since boot*
    // We need to convert this value into milliseconds and add it to the system boot time
    // The final result resolution is in milliseconds
    auto clocksPerMilliSec = (double) sysconf(_SC_CLK_TCK) / 1000.0;
    auto runningTime_ms = static_cast<uint64_t>((double) info.starttime / clocksPerMilliSec);
    auto bootTime_ms = static_cast<uint64_t>(getSystemBootTime() * 1000);
    uint64_t procStartTime = bootTime_ms + runningTime_ms;

    return procStartTime;
}


std::string createPidFile(const std::string &dir, const std::string &name)
{
    std::string fullPath = dir + "/" + name;
    std::ofstream fd(fullPath.c_str());
    if (!fd.good()) {
        throw SystemError("Failed to create the PID file");
    }

    fd << getpid() << std::endl;
    fd.close();

    return fullPath;
}

}
}

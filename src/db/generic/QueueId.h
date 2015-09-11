/*
 * Copyright (c) CERN 2015
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

#pragma once
#ifndef QUEUEID_H_
#define QUEUEID_H_

#include <string>

/// Per each link (identified by source and destination storages), each
/// VO has its own separate queue
/// The number of active transfers a link can sustain is decided independently,
/// and then split among the VOs with queued transfers for that link
class QueueId {
public:
    QueueId(const std::string& sourceSe, const std::string& destSe, const std::string& voName):
        sourceSe(sourceSe), destSe(destSe), voName(voName)
    {}

    std::string sourceSe;
    std::string destSe;
    std::string voName;
};

#endif // QUEUEID_H_

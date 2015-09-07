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

#ifndef LINKCONFIG_H_
#define LINKCONFIG_H_

#include <string>


class LinkConfig
{
public:
    LinkConfig() : numberOfStreams(2), tcpBufferSize(0), transferTimeout(3600) {};
    ~LinkConfig() {};

    std::string source;
    std::string destination;
    std::string state;
    std::string symbolicName;

    int numberOfStreams;
    int tcpBufferSize;
    int transferTimeout;

    std::string autoTuning;
};

#endif // LINKCONFIG_H_

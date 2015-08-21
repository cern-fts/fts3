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

#pragma once

#include <iostream>
#include <string>

class SeProtocolConfig
{
public:

    SeProtocolConfig(): symbolicName(""), NOSTREAMS(0),  TCP_BUFFER_SIZE(0), URLCOPY_TX_TO(0), NO_TX_ACTIVITY_TO(0)
    {
    }

    ~SeProtocolConfig()
    {
    }

    std::string symbolicName;
    int NOSTREAMS;
    int TCP_BUFFER_SIZE;
    int URLCOPY_TX_TO;
    int NO_TX_ACTIVITY_TO;

};

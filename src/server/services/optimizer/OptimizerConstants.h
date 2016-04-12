/*
 * Copyright (c) CERN 2013-2016
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

#ifndef FTS3_OPTIMIZERCONSTANTS_H
#define FTS3_OPTIMIZERCONSTANTS_H

namespace fts3 {
namespace optimizer {

    const double EMA_ALPHA = 0.7;
    const int MAX_SUCCESS_RATE = 100;
    const int MED_SUCCESS_RATE = 98;
    const int LOW_SUCCESS_RATE = 97;
    const int BASE_SUCCESS_RATE = 96;

    const int DEFAULT_MAX_ACTIVE_PER_LINK = 60;
    const int DEFAULT_MAX_ACTIVE_ENDPOINT_LINK = 60;

    const int DEFAULT_MIN_ACTIVE = 2;
    const int DEFAULT_LAN_ACTIVE = 10;
}
}

#endif //FTS3_OPTIMIZERCONSTANTS_H

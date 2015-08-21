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

#ifndef SNAPSHOT_H_
#define SNAPSHOT_H_

#include <string>
#include <utility>

#include <boost/optional.hpp>
#include <boost/property_tree/ptree.hpp>

namespace fts3
{
namespace cli
{

namespace pt = boost::property_tree;

class Snapshot
{
    friend class ResponseParser;

public:
    Snapshot() :
        vo(), src_se(), dst_se(), active(0), max_active(0), failed(0), finished(0), submitted(0),
        avg_queued(0), _15(0), _30(0), _5(0), _60(0), success_ratio(0), frequent_error()
    {

    }

    virtual ~Snapshot() {}

    void print(pt::ptree & out) const;

protected:

    std::string vo;

    std::string src_se;
    std::string dst_se;

    int active;
    int max_active;
    int failed;
    int finished;
    int submitted;
    int avg_queued;

    double _15;
    double _30;
    double _5;
    double _60;
    double success_ratio;


    std::pair<int,std::string> frequent_error;

};

} /* namespace cli */
} /* namespace fts3 */

#endif /* SNAPSHOT_H_ */

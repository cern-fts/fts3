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

#include "Snapshot.h"


namespace fts3
{
namespace cli
{

void Snapshot::print(pt::ptree & out) const
{
    out.put("active", active);
    out.put("avg_queued", avg_queued);

    pt::ptree avg_throughput;

    pt::ptree at15;
    at15.put("15", _15);
    avg_throughput.push_back(std::make_pair("", at15));

    pt::ptree at5;
    at5.put("5", _5);
    avg_throughput.push_back(std::make_pair("", at5));

    pt::ptree at30;
    at30.put("30", _30);
    avg_throughput.push_back(std::make_pair("", at30));

    pt::ptree at60;
    at60.put("60", _60);
    avg_throughput.push_back(std::make_pair("", at60));

    out.put_child("avg_throughput", avg_throughput);

    out.put("dest_se", dst_se);
    out.put("failed", failed);
    out.put("finished", finished);

    pt::ptree error;

    pt::ptree count;
    count.put("count", frequent_error.first);
    error.push_back(std::make_pair("", count));

    pt::ptree reason;
    reason.put("reason", frequent_error.second);
    error.push_back(std::make_pair("", reason));

    out.put_child("frequent_error", error);

    out.put("max_active", max_active);
    out.put("source_se", src_se);
    out.put("submitted", submitted);
    out.put("success_ratio", success_ratio);
    out.put("vo_name", vo);
}

} /* namespace cli */
} /* namespace fts3 */

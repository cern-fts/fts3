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

#include "MySqlAPI.h"
#include "common/Exceptions.h"

using namespace fts3::common;


void MySqlAPI::snapshot(const std::string & vo_name, const std::string & source_se_p, const std::string & dest_se_p, const std::string &, std::stringstream & result)
{
    soci::session sql(*connectionPool);

    std::string vo_name_local;
    std::string dest_se;
    std::string source_se;
    std::string reason;
    long long countReason = 0;
    long long active = 0;
    long long maxActive = 0;
    long long submitted = 0;
    double throughput1h = 0.0;
    double throughput30min = 0.0;
    double throughput15min = 0.0;
    double throughput5min = 0.0;
    long long nFailedLastHour = 0;
    long long  nFinishedLastHour = 0;
    double  ratioSuccessFailure = 0.0;
    std::string querySe;
    std::string queryVO;
    std::string querySeAll;
    std::string queryFileSePair;

    std::string dest_se_check;
    std::string source_se_check;

    int exists = 0;

    //get all se's
    querySeAll = "SELECT source_se, dest_se from t_optimize_active WHERE datetime >= (UTC_TIMESTAMP() - interval '60' minute) ";

    if(!vo_name.empty())
        querySe = " SELECT DISTINCT source_se, dest_se FROM t_job where vo_name='" + vo_name + "'";
    else
        querySe = " SELECT DISTINCT source_se, dest_se FROM t_file";

    time_t now = time(NULL);
    struct tm tTime;
    gmtime_r(&now, &tTime);

    soci::indicator isNull1 = soci::i_ok;
    soci::indicator isNull2 = soci::i_ok;
    soci::indicator isNull3 = soci::i_ok;
    soci::indicator isNull4 = soci::i_ok;
    soci::indicator isNull5 = soci::i_ok;
    soci::indicator isNull6 = soci::i_ok;


    if(vo_name.empty())
    {
        queryVO = "select distinct vo_name from t_job ";
    }

    bool sourceEmpty = true;

    if(!source_se_p.empty())
    {
        source_se = source_se_p;

        querySeAll += " and source_se = '" + source_se_p + "' ";

        if(!vo_name.empty())
        {
            querySe += " and source_se = '" + source_se + "'";
            queryVO += " and source_se = '" + source_se + "'";
        }
        else
        {
            querySe += " where source_se ='" + source_se + "'";
            queryVO += " where source_se ='" + source_se + "'";
        }
        sourceEmpty = false;
    }

    if(!dest_se_p.empty())
    {
        querySeAll += " and dest_se = '" + dest_se_p + "' ";

        if(sourceEmpty)
        {
            dest_se = dest_se_p;
            if(!vo_name.empty())
            {
                querySe += " and dest_se = '" + dest_se + "'";
                queryVO += " and dest_se = '" + dest_se + "'";
            }
            else
            {
                querySe += " where dest_se = '" + dest_se + "'";
                queryVO += " where dest_se = '" + dest_se + "'";
            }
        }
        else
        {
            dest_se = dest_se_p;
            querySe += " AND dest_se = '" + dest_se + "'";
            queryVO += " AND dest_se = '" + dest_se + "'";
        }
    }



    try
    {
        soci::statement pairsStmt((sql.prepare << querySe, soci::into(source_se), soci::into(dest_se)));
        soci::statement voStmt((sql.prepare << queryVO, soci::into(vo_name_local)));

        soci::statement st1((sql.prepare << "select count(*) from t_file where "
            " file_state='ACTIVE' and vo_name=:vo_name_local and "
            " source_se=:source_se and dest_se=:dest_se",
            soci::use(vo_name_local),
            soci::use(source_se),
            soci::use(dest_se),
            soci::into(active)));

        soci::statement st2((sql.prepare << "select active from t_optimize_active where "
            " source_se=:source_se and dest_se=:dest_se",
            soci::use(source_se),
            soci::use(dest_se),
            soci::into(maxActive, isNull1)
        ));

        soci::statement st3((sql.prepare << "select count(*) from t_file where "
            " file_state='SUBMITTED' and vo_name=:vo_name_local and "
            " source_se=:source_se and dest_se=:dest_se",
            soci::use(vo_name_local),
            soci::use(source_se),
            soci::use(dest_se),
            soci::into(submitted)
        ));


        soci::statement st41((sql.prepare << "select avg(throughput) from t_file where  "
            " source_se=:source_se and dest_se=:dest_se "
            " AND  file_state in ('ACTIVE','FINISHED') and (job_finished is NULL OR  job_finished >= (UTC_TIMESTAMP() - interval '60' minute)) "
            " AND throughput > 0 AND throughput is NOT NULL ",
            soci::use(source_se),
            soci::use(dest_se),
            soci::into(throughput1h, isNull2)
        ));

        soci::statement st42((sql.prepare << "select avg(throughput) from t_file where  "
            " source_se=:source_se and dest_se=:dest_se "
            " AND  file_state in ('ACTIVE','FINISHED') and (job_finished is NULL OR  job_finished >= (UTC_TIMESTAMP() - interval '30' minute)) "
            " AND throughput > 0 AND throughput is NOT NULL ",
            soci::use(source_se),
            soci::use(dest_se),
            soci::into(throughput30min, isNull3)
        ));

        soci::statement st43((sql.prepare << "select avg(throughput) from t_file where  "
            " source_se=:source_se and dest_se=:dest_se "
            " AND  file_state in ('ACTIVE','FINISHED') and (job_finished is NULL OR  job_finished >= (UTC_TIMESTAMP() - interval '15' minute)) "
            " AND throughput > 0 AND throughput is NOT NULL ",
            soci::use(source_se),
            soci::use(dest_se),
            soci::into(throughput15min, isNull4)
        ));

        soci::statement st44((sql.prepare << "select avg(throughput) from t_file where  "
            " source_se=:source_se and dest_se=:dest_se "
            " AND  file_state in ('ACTIVE','FINISHED') and (job_finished is NULL OR  job_finished >= (UTC_TIMESTAMP() - interval '5' minute)) "
            " AND throughput > 0 AND throughput is NOT NULL ",
            soci::use(source_se),
            soci::use(dest_se),
            soci::into(throughput5min, isNull5)
        ));


        soci::statement st5((sql.prepare << "select reason, count(reason) as c from t_file where "
            " (job_finished >= (UTC_TIMESTAMP() - interval '60' minute)) "
            " AND file_state='FAILED' and "
            " source_se=:source_se and dest_se=:dest_se and vo_name =:vo_name_local and reason is not null   "
            " group by reason order by c desc limit 1",
            soci::use(source_se),
            soci::use(dest_se),
            soci::use(vo_name_local),
            soci::into(reason, isNull6),
            soci::into(countReason)
        ));

        soci::statement st6((sql.prepare << "select count(*) from t_file where "
            " file_state='FAILED' and vo_name=:vo_name_local and "
            " source_se=:source_se and dest_se=:dest_se AND job_finished >= (UTC_TIMESTAMP() - interval '60' minute)",
            soci::use(vo_name_local),
            soci::use(source_se),
            soci::use(dest_se),
            soci::into(nFailedLastHour)
        ));

        soci::statement st7((sql.prepare << "select count(*) from t_file where "
            " file_state='FINISHED' and vo_name=:vo_name_local and "
            " source_se=:source_se and dest_se=:dest_se AND job_finished >= (UTC_TIMESTAMP() - interval '60' minute)",
            soci::use(vo_name_local),
            soci::use(source_se),
            soci::use(dest_se),
            soci::into(nFinishedLastHour)
        ));

        if(vo_name.empty())
        {
            result << "{\"snapshot\" : [";

            voStmt.execute();
            while (voStmt.fetch()) //distinct vo
            {
                if(source_se_p.empty())
                    source_se = "";
                if(dest_se_p.empty())
                    dest_se = "";

                bool first = true;

                pairsStmt.execute();
                while (pairsStmt.fetch()) //distinct source_se / dest_se
                {
                    active = 0;
                    maxActive = 0;
                    submitted = 0;
                    throughput1h = 0.0;
                    throughput30min = 0.0;
                    throughput15min = 0.0;
                    throughput5min = 0.0;
                    nFailedLastHour = 0;
                    nFinishedLastHour = 0;
                    ratioSuccessFailure = 0.0;


                    st1.execute(true);
                    st2.execute(true);
                    st7.execute(true);
                    st6.execute(true);
                    st3.execute(true);


                    //if all of the above return 0 then continue
                    if(active == 0 && nFinishedLastHour == 0 &&  nFailedLastHour == 0 && submitted == 0 && source_se_p.empty() && dest_se_p.empty())
                        continue;

                    if (!first) result << "\n,\n";
                    first = false;

                    result << "{\n";

                    result << std::fixed << "\"VO\":\"";
                    result <<   vo_name_local;
                    result <<   "\",\n";

                    result <<   "\"Source endpoint\":\"";
                    result <<   source_se;
                    result <<   "\",\n";

                    result <<   "\"Destination endpoint\":\"";
                    result <<   dest_se;
                    result <<   "\",\n";

                    //get active for this pair and vo
                    result <<   "\"Current active transfers\":\"";
                    result <<   active;
                    result <<   "\",\n";

                    //get max active for this pair no matter the vo
                    result <<   "\"Max active transfers\":\"";
                    if(isNull1 == soci::i_null)
                        result <<   0;
                    else
                        result <<   maxActive;
                    result <<   "\",\n";

                    result <<   "\"Number of finished (last hour)\":\"";
                    result <<   long(nFinishedLastHour);
                    result <<   "\",\n";

                    result <<   "\"Number of failed (last hour)\":\"";
                    result <<   long(nFailedLastHour);
                    result <<   "\",\n";

                    //get submitted for this pair and vo
                    result <<   "\"Number of queued\":\"";
                    result <<   submitted;
                    result <<   "\",\n";


                    //average throughput block
                    st41.execute(true);
                    result <<   "\"Avg throughput (last 60min)\":\"";
                    if(isNull2 == soci::i_null)
                        result <<   0;
                    else
                        result <<  std::setprecision(2) << throughput1h;
                    result <<   " MB/s\",\n";

                    st42.execute(true);
                    result <<   "\"Avg throughput (last 30min)\":\"";
                    if(isNull3 == soci::i_null)
                        result <<   0;
                    else
                        result <<  std::setprecision(2) << throughput30min;
                    result <<   " MB/s\",\n";

                    st43.execute(true);
                    result <<   "\"Avg throughput (last 15min)\":\"";
                    if(isNull4 == soci::i_null)
                        result <<   0;
                    else
                        result <<  std::setprecision(2) << throughput15min;
                    result <<   " MB/s\",\n";

                    st44.execute(true);
                    result <<   "\"Avg throughput (last 5min)\":\"";
                    if(isNull5 == soci::i_null)
                        result <<   0;
                    else
                        result <<  std::setprecision(2) << throughput5min;
                    result <<   " MB/s\",\n";


                    //round up efficiency
                    if(nFinishedLastHour > 0)
                    {
                        ratioSuccessFailure = (double)nFinishedLastHour/((double)nFinishedLastHour + (double)nFailedLastHour) * (100.0);
                    }

                    result <<   "\"Link efficiency (last hour)\":\"";
                    result <<   ratioSuccessFailure;
                    result <<   "%\",\n";

                    //most frequent error and number the last 30min
                    reason = "";
                    countReason = 0;
                    st5.execute(true);

                    result <<   "\"Most frequent error (last hour)\":\"";
                    if(isNull6 != soci::i_null)
                    {
                        result <<   countReason;
                        result <<   " times: ";
                        result <<   reason;
                    }
                    result <<   "\"\n";
                    result << "}\n";
                } //end distinct pair source_se / dest_se
            } //end distinct vo

            result << "]}";
        }//end vo empty
        else
        {
            vo_name_local = vo_name;
            if(source_se_p.empty())
                source_se = "";
            if(dest_se_p.empty())
                dest_se = "";


            soci::rowset<soci::row> rs = (
                sql.prepare << querySeAll
            );




            result << "{\"snapshot\" : [";

            bool first = true;

            for (soci::rowset<soci::row>::const_iterator i = rs.begin(); i != rs.end(); ++i)
            {
                source_se_check = i->get<std::string>("source_se");
                dest_se_check = i->get<std::string>("dest_se");
                exists = 0; //reset



                sql  << "select file_id from t_file where source_se= :source_se and dest_se=:dest_se and vo_name=:vo_name AND file_state IN ('FAILED', 'FINISHED', 'CANCELED', 'SUBMITTED', 'ACTIVE') LIMIT 1",
                    soci::use(source_se_check),
                    soci::use(dest_se_check),
                    soci::use(vo_name_local),
                    soci::into(exists);

                if(exists > 0)
                {
                    active = 0;
                    maxActive = 0;
                    submitted = 0;
                    throughput1h = 0.0;
                    throughput30min = 0.0;
                    throughput15min = 0.0;
                    throughput5min = 0.0;
                    nFailedLastHour = 0;
                    nFinishedLastHour = 0;
                    ratioSuccessFailure = 0.0;
                    source_se = source_se_check;
                    dest_se = dest_se_check;

                    st1.execute(true);
                    st2.execute(true);
                    st7.execute(true);
                    st6.execute(true);
                    st3.execute(true);


                    //if all of the above return 0 then continue
                    if(active == 0 && nFinishedLastHour == 0 &&  nFailedLastHour == 0 && submitted == 0 && source_se_p.empty() && dest_se_p.empty())
                        continue;

                    if (!first) result << "\n,\n";
                    first = false;

                    result << "{\n";

                    result << std::fixed << "\"VO\":\"";
                    result <<   vo_name_local;
                    result <<   "\",\n";

                    result <<   "\"Source endpoint\":\"";
                    result <<   source_se;
                    result <<   "\",\n";

                    result <<   "\"Destination endpoint\":\"";
                    result <<   dest_se;
                    result <<   "\",\n";

                    //get active for this pair and vo
                    result <<   "\"Current active transfers\":\"";
                    result <<   active;
                    result <<   "\",\n";

                    //get max active for this pair no matter the vo
                    result <<   "\"Max active transfers\":\"";
                    if(isNull1 == soci::i_null)
                        result <<   0;
                    else
                        result <<   maxActive;
                    result <<   "\",\n";

                    result <<   "\"Number of finished (last hour)\":\"";
                    result <<   long(nFinishedLastHour);
                    result <<   "\",\n";

                    result <<   "\"Number of failed (last hour)\":\"";
                    result <<   long(nFailedLastHour);
                    result <<   "\",\n";

                    //get submitted for this pair and vo
                    result <<   "\"Number of queued\":\"";
                    result <<   submitted;
                    result <<   "\",\n";


                    //average throughput block
                    st41.execute(true);
                    result <<   "\"Avg throughput (last 60min)\":\"";
                    if(isNull2 == soci::i_null)
                        result <<   0;
                    else
                        result <<  std::setprecision(2) << throughput1h;
                    result <<   " MB/s\",\n";

                    st42.execute(true);
                    result <<   "\"Avg throughput (last 30min)\":\"";
                    if(isNull3 == soci::i_null)
                        result <<   0;
                    else
                        result <<  std::setprecision(2) << throughput30min;
                    result <<   " MB/s\",\n";

                    st43.execute(true);
                    result <<   "\"Avg throughput (last 15min)\":\"";
                    if(isNull4 == soci::i_null)
                        result <<   0;
                    else
                        result <<  std::setprecision(2) << throughput15min;
                    result <<   " MB/s\",\n";

                    st44.execute(true);
                    result <<   "\"Avg throughput (last 5min)\":\"";
                    if(isNull5 == soci::i_null)
                        result <<   0;
                    else
                        result <<  std::setprecision(2) << throughput5min;
                    result <<   " MB/s\",\n";


                    //round up efficiency
                    if(nFinishedLastHour > 0)
                    {
                        ratioSuccessFailure = (double)nFinishedLastHour/((double)nFinishedLastHour + (double)nFailedLastHour) * (100.0);
                    }

                    result <<   "\"Link efficiency (last hour)\":\"";
                    result <<   ratioSuccessFailure;
                    result <<   "%\",\n";

                    //most frequent error and number the last 30min
                    reason = "";
                    countReason = 0;
                    st5.execute(true);

                    result <<   "\"Most frequent error (last hour)\":\"";
                    if(isNull6 != soci::i_null)
                    {
                        result <<   countReason;
                        result <<   " times: ";
                        result <<   reason;
                    }
                    result <<   "\"\n";

                    result << "}\n";
                }
            }

            result << "]}";
        }
        result.unsetf(std::ios::floatfield);
    }
    catch (std::exception& e)
    {
        throw UserError(std::string(__func__) + ": Caught exception " + e.what());
    }
    catch (...)
    {
        throw UserError(std::string(__func__) + ": Caught exception ");
    }
}

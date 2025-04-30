/*
 * Copyright (c) CERN 2024
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

/**
* An FTS file-transfer queue within the t_queue table can be identified in two
* different ways.  Firstly it can be identified by the unique t_queue.queue_id
* column.  This is the preferred way to identify a queue when it already exists
* within the database, for example when you want to decrement its
* t_queue.nb_files counter/column.  The second way to identify a queue is by
* combining the following "identifying" attributes:
*
*     t_queue.vo_name
*     t_queue.source_se
*     t_queue.dest_se
*     t_queue.activity
*     t_queue.file_state
*
* This composite identiier is necessary when the queue does not yet exist in the
* database.
*
* This structure represents a composite queue identifier.
*/
struct QueueCompId
{
    std::string vo_name;
    std::string source_se;
    std::string dest_se;
    std::string activity;
    std::string file_state;
};

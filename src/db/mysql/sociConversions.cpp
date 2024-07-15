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

#include "MySqlAPI.h"
#include "sociConversions.h"

unsigned long long get_file_id_from_row(const soci::row &row)
{
    const auto file_id_props = row.get_properties("file_id");

    if (soci::dt_unsigned_long_long == file_id_props.get_data_type()) {
        return row.get<unsigned long long>("file_id");
    } else if (soci::dt_long_long == file_id_props.get_data_type()) {
        const long long tmp_file_id = row.get<long long>("file_id");
        if (0 > tmp_file_id) {
            std::ostringstream msg;
            msg << "Invalid t_file.file_id value: Value is negative: file_id=" << tmp_file_id;
            throw std::runtime_error(msg.str());
        }
        return (unsigned long long)tmp_file_id;
    } else {
        throw std::runtime_error(
            "Invalid t_file.file_id value: Value has an unknown type"
        );
    }
}

unsigned long long get_file_id_from_values(const soci::values &values)
{
    const auto file_id_props = values.get_properties("file_id");

    if (soci::dt_unsigned_long_long == file_id_props.get_data_type()) {
        return values.get<unsigned long long>("file_id");
    } else if (soci::dt_long_long == file_id_props.get_data_type()) {
        const long long tmp_file_id = values.get<long long>("file_id");
        if (0 > tmp_file_id) {
            std::ostringstream msg;
            msg << "Invalid t_file.file_id value: Value is negative: file_id=" << tmp_file_id;
            throw std::runtime_error(msg.str());
        }
        return (unsigned long long)tmp_file_id;
    } else {
        throw std::runtime_error(
            "Invalid t_file.file_id value: Value has an unknown type"
        );
    }
}

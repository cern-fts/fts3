/*
 * Copyright (c) CERN 2021
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

#include <json/json.h>
#include "heuristics.h"

class DestFile {

public:
    uint64_t fileSize;
    std::string checksumType;
    std::string checksumValue;
    bool fileOnDisk;
    bool fileOnTape;

    DestFile() : fileSize(0), fileOnDisk(false), fileOnTape(false) {}

    Json::Value toJSON() const
    {
        Json::Value output;

        output["file_size"] = (Json::Value::UInt64) fileSize;
        output["checksum_type"] = checksumType;
        output["checksum_value"] = checksumValue;
        output["file_on_tape"] = fileOnTape;
        output["file_on_disk"] = fileOnDisk;
        
        return output;
    }

    std::string toString()
    {
        std::ostringstream oss;

        oss << "file_size=" << fileSize << " checksum=" << checksumType << ":" << checksumValue << " "
            << "file_on_disk=" << fileOnDisk << " file_on_tape=" << fileOnTape;

        return oss.str();
    }

    static std::string appendDestFileToFileMetadata(const std::string& file_metadata, const Json::Value& dst_file)
    {
        Json::Value metadata;

        if (!file_metadata.empty()) {
            try {
                auto new_file_metadata = replaceMetadataString(file_metadata);
                std::istringstream valueStream(new_file_metadata);
                valueStream >> metadata;
            }
            catch (...) {
                metadata["file_metadata"] = file_metadata;
            }
        }
        
        metadata["dst_file"] = dst_file;
        
        std::ostringstream stream;
        stream << metadata;

        return stream.str();
    }
};

#include <cajun/json/elements.h>
#include <cajun/json/writer.h>
#include <cajun/json/reader.h>
#include "heuristics.h"


class DestFile {

public:
    uint64_t fileSize;
    std::string checksumType;
    std::string checksumValue;
    bool fileOnDisk;
    bool fileOnTape;

    DestFile() : fileSize(0), fileOnDisk(false), fileOnTape(false) {}

    json::Object toJSON()
    {    
        json::Object output;
        
        output["file_size"] = json::Number(fileSize);
        output["checksum_type"] = json::String(checksumType);
        output["checksum_value"] = json::String(checksumValue);
        output["file_on_tape"] = json::Boolean(fileOnDisk);
        output["file_on_disk"] = json::Boolean(fileOnTape);
        
        return output;
    }

    static std::string appendDestFileToFileMetadata(std::string file_metadata, json::Object dst_file) 
    {
        json::UnknownElement metadata;

        if (!file_metadata.empty()) {
            try {
                std::string new_file_metadata = replaceMetadataString(file_metadata);
                std::istringstream valueStream(new_file_metadata);
                json::Reader::Read(metadata, valueStream);
            }
            catch (...) {
                metadata["file_metada"] = json::String(file_metadata);
            }
        }
        
        metadata["dst_file"] = json::Object(dst_file);
        
        std::ostringstream stream;
        json::Writer::Write(metadata, stream);

        return stream.str();
    }
};
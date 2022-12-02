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
        output["file_on_tape"] = json::Boolean(fileOnTape);
        output["file_on_disk"] = json::Boolean(fileOnDisk);
        
        return output;
    }

    std::string toString()
    {
        std::ostringstream oss;

        oss << "file_size=" << fileSize << " checksum=" << checksumType << ":" << checksumValue << " "
            << "file_on_disk=" << fileOnDisk << " file_on_tape=" << fileOnTape;

        return oss.str();
    }

    static std::string appendDestFileToFileMetadata(const std::string& file_metadata, json::Object dst_file)
    {
        json::UnknownElement metadata;

        if (!file_metadata.empty()) {
            try {
                auto new_file_metadata = replaceMetadataString(file_metadata);
                std::istringstream valueStream(new_file_metadata);
                json::Reader::Read(metadata, valueStream);
            }
            catch (...) {
                metadata["file_metadata"] = json::String(file_metadata);
            }
        }
        
        metadata["dst_file"] = json::Object(std::move(dst_file));
        
        std::ostringstream stream;
        json::Writer::Write(metadata, stream);

        return stream.str();
    }
};
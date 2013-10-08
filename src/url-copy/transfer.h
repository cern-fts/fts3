#pragma once

#include <string>

class Transfer {
public:
    unsigned    fileId;
    std::string jobId;
    std::string sourceUrl;
    std::string destUrl;
    std::string checksumAlgorithm;
    std::string checksumValue;
    double      userFileSize;
    std::string fileMetadata;
    std::string tokenBringOnline;

    Transfer(): fileId(0), userFileSize(0)
    {
    }

    void setChecksum(const std::string& checksum)
    {
        size_t colon = checksum.find(':');
        if (colon == std::string::npos) {
            checksumAlgorithm.assign(checksum);
            checksumValue.clear();
        }
        else {
            checksumAlgorithm.assign(checksum.substr(0, colon));
            checksumValue.assign(checksum.substr(colon + 1));
        }
    }
};

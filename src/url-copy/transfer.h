#pragma once

#include <stdint.h>
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

    // Timestamps in milliseconds
    uint64_t startTime;
    uint64_t finishTime;

    Transfer(): fileId(0), userFileSize(0), startTime(0), finishTime(0)
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

    double getTransferDurationInSeconds()
    {
        if (startTime == 0 || finishTime == 0)
            return 0.0;
        return (static_cast<double>(finishTime) - static_cast<double>(startTime)) / 1000.0;
    }
};

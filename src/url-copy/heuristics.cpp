#include <errno.h>
#include <limits.h>
#include "common/definitions.h"
#include "heuristics.h"



bool retryTransfer(int errorNo, const std::string& category, const std::string& message)
{
    bool retry = true;

    if (category == "SOURCE")
        {
            switch (errorNo)
                {
                case ENOENT:          // No such file or directory
                case EPERM:           // Operation not permitted
                case EACCES:          // Permission denied
                case EISDIR:          // Is a directory
                case ENAMETOOLONG:    // File name too long
                case E2BIG:           // Argument list too long
                case ENOTDIR:         // Part of the path is not a directory
                case EPROTONOSUPPORT: // Protocol not supported by gfal2 (plugin missing?)
                    retry = false;
                    break;
                default:
                    retry = true;
                    break;
                }
        }
    else if (category == "DESTINATION")
        {
            switch (errorNo)
                {
                case EPERM:           // Operation not permitted
                case EACCES:          // Permission denied
                case EISDIR:          // Is a directory
                case ENAMETOOLONG:    //  File name too long
                case E2BIG:           //  Argument list too long
                case EPROTONOSUPPORT: // Protocol not supported by gfal2 (plugin missing?)
                    retry = false;
                    break;
                default:
                    retry = true;
                    break;
                }
        }
    else //TRANSFER
        {
            switch (errorNo)
                {
                case ENOSPC:          // No space left on device
                case EPERM:           // Operation not permitted
                case EACCES:          // Permission denied
                case EEXIST:          // File exists
                case EFBIG:           // File too big
                case EROFS:           // Read-only file system
                case ENAMETOOLONG:    // File name too long
                case EPROTONOSUPPORT: // Protocol not supported by gfal2 (plugin missing?)
                    retry = false;
                    break;
                default:
                    retry = true;
                    break;
                }
        }

    //search for error patterns not reported by SRM or GSIFTP
    std::size_t found = message.find("proxy expired");
    if (found!=std::string::npos)
        retry = false;
    found = message.find("File exists and overwrite");
    if (found!=std::string::npos)
        retry = false;
    found = message.find("No such file or directory");
    if (found!=std::string::npos)
        retry = false;
    found = message.find("SRM_INVALID_PATH");
    if (found!=std::string::npos)
        retry = false;
    found = message.find("The certificate has expired");
    if (found!=std::string::npos)
        retry = false;
    found = message.find("The available CRL has expired");
    if (found!=std::string::npos)
        retry = false;
    found = message.find("SRM Authentication failed");
    if (found!=std::string::npos)
        retry = false;
    found = message.find("SRM_DUPLICATION_ERROR");
    if (found!=std::string::npos)
        retry = false;

    return retry;
}



unsigned adjustStreamsBasedOnSize(off_t sizeInBytes, unsigned int /*currentStreams*/)
{
    if (sizeInBytes <= 5242880) //starting with 5MB
        return 1;
    else if (sizeInBytes <= 10485760)
        return 2;
    else if (sizeInBytes > 10485760 && sizeInBytes <= 52428800)
        return 3;
    else if (sizeInBytes > 52428800 && sizeInBytes <= 104857600)
        return 4;
    else if (sizeInBytes > 104857600 && sizeInBytes <= 209715200)
        return 5;
    else if (sizeInBytes > 209715200 && sizeInBytes <= 734003200)
        return 6;
    else if (sizeInBytes > 734003200 && sizeInBytes <= 1073741824)
        return 7;
    else if (sizeInBytes > 1073741824 && sizeInBytes <= 1610612736)
        return 8;
    else if (sizeInBytes > 1610612736 && sizeInBytes <= 2010612736)
        return 9;
    else if (sizeInBytes > 2010612736 && sizeInBytes <= 2576980377)
        return 10;
    else if (sizeInBytes > 2576980377 && sizeInBytes <= 3758096384)
        return 12;
    else if (sizeInBytes > 3758096384 && sizeInBytes <= 4858096384)
        return 14;
    else
        return 16;
    return 6;
}



static unsigned adjustTimeout(off_t sizeInBytes)
{
    if (sizeInBytes <= 10485760) //starting with 10MB
        return 900;
    else if (sizeInBytes > 10485760 && sizeInBytes <= 52428800)
        return 1100;
    return 3600;
}



unsigned adjustTimeoutBasedOnSize(off_t sizeInBytes,
                                  unsigned timeout)
{
    static const unsigned long MB = 1 << 20;

    // Reasonable time to wait per MB transferred
    // If input timeout is 0, give it a little more room
    long double timeoutPerMB = 2;
    if (timeout == 0)
        timeoutPerMB = 5;

    // Reasonable time to wait for the transfer itself
    unsigned transferTimeout = adjustTimeout(sizeInBytes);

    // Final timeout adjusted considering transfer timeout
    long double totalTimeout = transferTimeout + ceil(timeoutPerMB * sizeInBytes / MB);

    // Truncate to an unsigned
    if (totalTimeout >= INT_MAX)
        timeout = 4000;
    else
        timeout = static_cast<unsigned>(totalTimeout);

    if(timeout > 30000)
        timeout = 30000;

    return timeout;
}

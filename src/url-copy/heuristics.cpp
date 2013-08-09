#include <errno.h>
#include <limits.h>
#include "common/definitions.h"
#include "heuristics.h"



bool retryTransfer(int errorNo, const std::string& category)
{
    bool retry = true;

    if (category == "SOURCE")
        {
            switch (errorNo)
                {
                case ENOENT:       // No such file or directory
                case EPERM:        // Operation not permitted
                case EACCES:       // Permission denied
                case EISDIR:       // Is a directory
                case ENAMETOOLONG: // File name too long
                case E2BIG:        // Argument list too long
                case ENOTDIR:      // Part of the path is not a directory
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
                case EPERM:        // Operation not permitted
                case EACCES:       // Permission denied
                case EISDIR:       // Is a directory
                case ENAMETOOLONG: //  File name too long
                case E2BIG:        //  Argument list too long
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
                case ENOSPC:       // No space left on device
                case EPERM:        // Operation not permitted
                case EACCES:       // Permission denied
                case EEXIST:       // File exists
                case EFBIG:        // File too big
                case EROFS:        // Read-only file system
                case ENAMETOOLONG: // File name too long
                    retry = false;
                    break;
                default:
                    retry = true;
                    break;
                }
        }

    return retry;
}



unsigned adjustStreamsBasedOnSize(off_t sizeInBytes, unsigned int currentStreams)
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
    else if (currentStreams < 10)
        return 10;
    return 6;
}



static unsigned adjustTimeout(off_t sizeInBytes)
{
    if (sizeInBytes <= 10485760) //starting with 10MB
        return 900;
    else if (sizeInBytes > 10485760 && sizeInBytes <= 52428800)
        return 1100;
    return 3200;
}



unsigned adjustTimeoutBasedOnSize(off_t sizeInBytes,
                                  unsigned timeout)
{
    static const unsigned long MB = 1 << 20;

    // Reasonable time to wait per MB transferred
    // If input timeout is 0, give it a little more room
    long double timeoutPerMB = 2;
    if (timeout == 0)
        timeoutPerMB = 3;

    // Reasonable time to wait for the transfer itself
    unsigned transferTimeout = adjustTimeout(sizeInBytes);

    // Final timeout adjusted considering transfer timeout
    long double totalTimeout = transferTimeout + ceil(timeoutPerMB * sizeInBytes / MB);

    // Truncate to an unsigned
    if (totalTimeout >= INT_MAX)
        timeout = 4000;
    else
        timeout = static_cast<unsigned>(totalTimeout);

    if(timeout > 6000)
        timeout = 6000;

    return timeout;
}

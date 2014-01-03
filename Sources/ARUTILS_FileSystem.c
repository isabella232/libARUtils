/**
 * @file ARUTILS_FileSystem.c
 * @brief libARUtils FileSystem c file.
 * @date 19/12/2013
 * @author david.flattin.ext@parrot.com
 **/

#include "config.h"
#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#ifdef HAVE_SYS_STATFS_H
#include <sys/statfs.h> //linux
#endif
#ifdef HAVE_SYS_MOUNT_H
#include <sys/mount.h> //ios
#endif

#include <libARSAL/ARSAL_Print.h>
#include <libARSAL/ARSAL_Ftw.h>

#include "libARUtils/ARUTILS_Error.h"
#include "libARUtils/ARUTILS_FileSystem.h"

#define ARUTILS_FILE_SYSTEM_TAG   "FileSystem"

/*****************************************
 *
 *             Private implementation:
 *
 *****************************************/

eARUTILS_ERROR ARUTILS_FileSystem_GetFileSize(const char *namePath, uint32_t *size)
{
    struct stat statbuf = { 0 };
    uint32_t fileSize = 0;
    eARUTILS_ERROR result = ARUTILS_OK;
    int resultSys = 0;

    ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARUTILS_FILE_SYSTEM_TAG, "%s", namePath ? namePath : "null");

    if (namePath == NULL)
    {
        result = ARUTILS_ERROR_BAD_PARAMETER;
    }

    resultSys = stat(namePath, &statbuf);

    if (resultSys == 0)
    {
        if (S_ISREG(statbuf.st_mode))
        {
            fileSize = (uint32_t)statbuf.st_size;
        }
    }
    else
    {
        result = ARUTILS_ERROR_SYSTEM;

        if (ENOENT == errno)
        {
            result = ARUTILS_ERROR_FILE_NOT_FOUND;
        }
    }

    *size = fileSize;
    return result;
}

eARUTILS_ERROR ARUTILS_FileSystem_Rename(const char *oldName, const char *newName)
{
    eARUTILS_ERROR result = ARUTILS_OK;
    int resultSys = 0;

    ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARUTILS_FILE_SYSTEM_TAG, "%s, %s", oldName ? oldName : "null", newName ? newName : "null");

    resultSys = rename(oldName, newName);

    if (resultSys != 0)
    {
        result = ARUTILS_ERROR_SYSTEM;
    }

    return result;
}

eARUTILS_ERROR ARUTILS_FileSystem_RemoveFile(const char *localPath)
{
    eARUTILS_ERROR result = ARUTILS_OK;
    int resultSys = 0;

    ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARUTILS_FILE_SYSTEM_TAG, "%s", localPath ? localPath : "null");

    resultSys = remove(localPath);

    if (resultSys != 0)
    {
        result = ARUTILS_ERROR_SYSTEM;
    }

    return result;
}

int ARUTILS_FileSystem_RemoveDirCallback(const char* fpath, const struct stat *sb, int typeflag)
{
	if(typeflag == FTW_F)
    {
		remove(fpath);
    }
    else if(typeflag == FTW_D)
    {
        rmdir(fpath);
    }

	return 0;
}

eARUTILS_ERROR ARUTILS_FileSystem_RemoveDir(const char *localPath)
{
    eARUTILS_ERROR result = ARUTILS_OK;

    ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARUTILS_FILE_SYSTEM_TAG, "%s", localPath ? localPath : "null");

    int resultSys = ARSAL_Ftw(localPath, ARUTILS_FileSystem_RemoveDirCallback, ARUTILS_FILE_SYSTEM_MAX_FD_FOR_FTW);

    if (resultSys == 0)
    {
        resultSys = rmdir(localPath);

        if (resultSys != 0)
        {
            result = ARUTILS_ERROR_SYSTEM;
        }
    }

    return result;
}

eARUTILS_ERROR ARUTILS_FileSystem_GetFreeSpace(const char *localPath, double *freeSpace)
{
    eARUTILS_ERROR result = ARUTILS_OK;
    struct statfs statfsData;
    double freeBytes = 0.f;

    ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARUTILS_FILE_SYSTEM_TAG, "%s", localPath ? localPath : "null");

    int resultSys = statfs(localPath, &statfsData);

    if (resultSys != 0)
    {
        result = ARUTILS_ERROR_SYSTEM;
    }
    else
    {
        freeBytes = ((double)statfsData.f_bavail) * ((double)statfsData.f_bsize);
    }

    *freeSpace = freeBytes;
    return result;
}









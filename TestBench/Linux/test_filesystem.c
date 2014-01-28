/**
 * @file test_ftp_connection.c
 * @brief libARUtils test ftp connection c file.
 * @date 19/12/2013
 * @author david.flattin.ext@parrot.com
 */

#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <sys/stat.h>
#ifdef HAVE_SYS_STATFS_H
#include <sys/statfs.h> //linux
#endif
#ifdef HAVE_SYS_MOUNT_H
#include <sys/mount.h> //ios
#endif

#include <libARSAL/ARSAL_Sem.h>
#include <libARUtils/ARUTILS_Error.h>
#include <libARUtils/ARUTILS_FileSystem.h>

void test_filesystem(const char *tmp)
{
    char localPath[512];
    int ret;
    
    strcpy(localPath, tmp);
    strcat(localPath, "/aa");
    ret = mkdir (localPath, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    
    strcat(localPath, "/bb");
    ret = mkdir (localPath, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    
    strcat(localPath, "/cc");
    ret = mkdir (localPath, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    
    strcpy(localPath, tmp);
    strcat(localPath, "/aa");
    
    ret = ARUTILS_FileSystem_RemoveDir(localPath);
    
    ret = ARUTILS_FileSystem_RemoveDir(localPath);
}
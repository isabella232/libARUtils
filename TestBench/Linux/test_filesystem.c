/*
    Copyright (C) 2014 Parrot SA

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in
      the documentation and/or other materials provided with the 
      distribution.
    * Neither the name of Parrot nor the names
      of its contributors may be used to endorse or promote products
      derived from this software without specific prior written
      permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
    "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
    LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
    FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
    COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
    INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
    BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
    OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED 
    AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
    OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
    OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
    SUCH DAMAGE.
*/
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
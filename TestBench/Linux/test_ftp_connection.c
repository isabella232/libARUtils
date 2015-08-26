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

#include <libARSAL/ARSAL_Sem.h>
#include <libARUtils/ARUTILS_Error.h>
#include <libARUtils/ARUTILS_Ftp.h>

#include <curl/curl.h>
#include "ARUTILS_WifiFtp.h"

#define DEVICE_IP    "172.20.5.117"

void text_list(const char *tmp)
{
    ARUTILS_WifiFtp_Connection_t *connection = NULL;
    eARUTILS_ERROR result;
    ARSAL_Sem_t cancelSem;
    char *resultList = NULL;
    
    uint32_t resultListLen = 0;
    //char *nextItem = NULL;
    //char *item;
    
    ARSAL_Sem_Init(&cancelSem, 0, 0);
    
    connection = ARUTILS_WifiFtp_Connection_New(&cancelSem, DEVICE_IP, 21, "anonymous", "", &result);
    printf("result %d\n", result);
    
    result = ARUTILS_WifiFtp_List(connection, "", &resultList, &resultListLen);
    printf("result %d\n", result);
    printf("%s\n", resultList ? resultList : "null");
    
    /*while ((item = List_GetNextItem(resultList, &nextItem, 0)))
    {
        printf("%s\n", item);
    }*/
    
    free(resultList);
}

void test_ftp_progress_callback(void* arg, float percent)
{
    char *message = (char *)arg;
    
    printf("%s %02f%%\n", message ? message : "null", percent);
}

void test_ftp_commands(char *tmp)
{
    ARUTILS_WifiFtp_Connection_t *connection = NULL;
    eARUTILS_ERROR result;
    ARSAL_Sem_t cancelSem;
    char *resultList = NULL;
    uint32_t resultListLen = 0;
    double fileSize = 0.f;
    
    ARSAL_Sem_Init(&cancelSem, 0, 0);
    
    connection = ARUTILS_WifiFtp_Connection_New(&cancelSem, DEVICE_IP, 21, "anonymous", "", &result);
    printf("result %d\n", result);
    
    result = ARUTILS_WifiFtp_List(connection, "", &resultList, &resultListLen);
    printf("result %d\n", result);
    printf("%s\n", resultList ? resultList : "null");
    free(resultList);
    
    result = ARUTILS_WifiFtp_Connection_Cancel(connection);
    printf("result %d\n", result);
    
    //result = ARUTILS_WifiFtp_Cd(connection, "a");
    //printf("result %d\n", result);
    
    result = ARUTILS_WifiFtp_List(connection, "/b", &resultList, &resultListLen);
    printf("result %d\n", result);
    printf("%s\n", resultList ? resultList : "null");
    free(resultList);
    
    result = ARUTILS_WifiFtp_Size(connection, "a/text0.txt", &fileSize);
    //result = ARUTILS_WifiFtp_Size(connection, "test.txt", &fileSize);
    printf("result %d\n", result);
    printf("size %.0f\n", (float)fileSize);
    //CURLINFO_NEW_FILE_PERMS
    //CURLINFO_NEW_DIRECTORY_PERMS
    
    char localFilePath[512];
    //int64_t localSize = 0;
    strcpy(localFilePath, tmp);
    strcat(localFilePath, "text0.txt");
    
    result = ARUTILS_WifiFtp_Get(connection, "a/text0.txt", localFilePath, test_ftp_progress_callback, "progress: ", FTP_RESUME_FALSE);
    //result = ARUTILS_WifiFtp_Get(connection, "a/text0.txt", localFilePath, test_ftp_progress_callback, "progress: ", FTP_RESUME_TRUE);
    printf("result %d\n", result);
    
    //result = ARUTILS_FileSystem_GetFileSize(localFilePath, &localSize);
    //printf("result %d\n", result);
    
    result = ARUTILS_WifiFtp_Put(connection, "a/text00.txt", localFilePath, test_ftp_progress_callback, "progress: ", 0);
    //result = ARUTILS_WifiFtp_Put(connection, "a/text00.txt", localFilePath, test_ftp_progress_callback, "progress: ", 1);
    printf("result %d\n", result);
    
    result = ARUTILS_WifiFtp_Size(connection, "a/text00.txt", &fileSize);
    printf("result %d\n", result);
    printf("size %.0f\n", (float)fileSize);
    
    result = ARUTILS_WifiFtp_Delete(connection, "a/text00.txt");
    printf("result %d\n", result);
    
    result = ARUTILS_WifiFtp_Size(connection, "a/text00.txt", &fileSize);
    printf("result %d\n", result);
    printf("size %.0f\n", (float)fileSize);
    
    result = ARUTILS_WifiFtp_Get(connection, "a/text00.txt", localFilePath, NULL, NULL, 0);
    printf("result %d\n", result);
    
    result = ARUTILS_WifiFtp_List(connection, "/c", &resultList, &resultListLen);
    printf("result %d\n", result);
    printf("%s\n", resultList ? resultList : "null");
    free(resultList);
    
    
    ARUTILS_WifiFtp_Connection_Delete(&connection);
}

void test_ftp_connection(char *tmp)
{
    text_list(tmp);
    
    test_ftp_commands(tmp);
}